// Motor do instalador: deteccao do jogo, leitura/verificacao do pacote (payload), instalacao com
// backup e desinstalacao que restaura o backup byte a byte. Sem interface: a janela (Janela.cs) e
// o modo silencioso (Programa.cs) chamam estas classes e recebem o andamento por IRelato.
//
// Regras (as do mod/Install-Game.ps1 de setembro, que este motor substituiu; o script saiu em 22/10, review M-12):
//  - nada fora da pasta do jogo e escrito (o download do REFramework vai para %TEMP%\AMDLSS5-Instalador);
//  - todo arquivo do jogo que seria sobrescrito vai antes para <pasta do exe>\AMDLSS5_backup\<data>\;
//  - acoes.json e regravado a cada passo (antes da escrita arriscada): uma falha no meio desfaz sozinha;
//  - INI do jogador (ja existia antes do mod) nunca e sobrescrito: so recebe as chaves que faltam, na secao e na
//    codificacao dele; INI que o mod criou numa versao anterior vira o do pacote com as chaves do jogador (Ini.ChaveDoUsuario);
//  - dinput8.dll alheio (REFramework do usuario ou outro mod) nunca e trocado;
//  - desinstalar aplica as sessoes de backup da mais nova para a mais velha e so apaga um arquivo
//    acrescentado se ele ainda e exatamente o que o instalador pos (senao e do usuario: fica).
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;

namespace Amdlss5.Instalador
{
    public interface IRelato
    {
        void Linha(string texto);
        void Progresso(int porcento);
    }

    public enum CodigoErro { Geral = 1, Bloqueado = 2, JogoAberto = 3, Permissao = 4, Faltando = 5 }

    public sealed class ErroInstalador : Exception
    {
        public readonly CodigoErro Codigo;
        public ErroInstalador(CodigoErro codigo, string mensagem) : base(mensagem) { Codigo = codigo; }
    }

    public static class Util
    {
        public const string PastaBackup = "AMDLSS5_backup";
        public const string NomeIni = "dlssnr_on_amd.ini";
        public const string NomeProxy = "version.dll";
        public const string NomeHelper = "nr_native_adapter.dll";
        public const string NomeNvngx = "nvngx_dlssnr.dll";
        public const string NomePesos = "dlssnr_on_amd_weights.bin";
        public static readonly string[] ArquivosNvidia = { NomeNvngx, NomePesos };
        // 1.1.0: arquivos que o mod cria ao jogar (nao vem no pacote); a desinstalacao devolve a pasta como era.
        public static readonly string[] ArquivosDoMod = { NomeIni, "amdlss5.dat", "nr_orcamento.ini" };   // 1.3.0: calibracao da VRAM que o mod grava ao jogar

        public static string Sha256(string caminho)
        {
            using (SHA256 h = SHA256.Create())
            using (FileStream fs = new FileStream(caminho, FileMode.Open, FileAccess.Read, FileShare.Read, 1 << 20))
            {
                byte[] d = h.ComputeHash(fs);
                StringBuilder sb = new StringBuilder(64);
                foreach (byte b in d) sb.Append(b.ToString("X2"));
                return sb.ToString();
            }
        }

        public static string Curto(string sha) { return string.IsNullOrEmpty(sha) ? "--------" : sha.Substring(0, Math.Min(8, sha.Length)); }

        public static string Mb(long bytes) { return (bytes / 1048576.0).ToString("0.0") + " MB"; }

        public static bool Dentro(string caminho, string pasta)
        {
            string p = Path.GetFullPath(pasta).TrimEnd('\\') + "\\";
            return Path.GetFullPath(caminho).StartsWith(p, StringComparison.OrdinalIgnoreCase);
        }

        public static string TempInstalador()
        {
            string d = Path.Combine(Path.GetTempPath(), "AMDLSS5-Instalador");
            Directory.CreateDirectory(d);
            return d;
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr OpenProcess(uint acesso, bool herda, int pid);
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern bool QueryFullProcessImageNameW(IntPtr h, int flags, StringBuilder texto, ref int tamanho);
        [DllImport("kernel32.dll")]
        static extern bool CloseHandle(IntPtr h);

        // Processos cujo executavel esta dentro da pasta (jogo aberto). Pelo caminho da imagem, nunca
        // por nome: vale para qualquer jogo.
        public static List<string> ProcessosNaPasta(string pasta)
        {
            List<string> achados = new List<string>();
            foreach (Process p in Process.GetProcesses())
            {
                try
                {
                    IntPtr h = OpenProcess(0x1000, false, p.Id); // PROCESS_QUERY_LIMITED_INFORMATION
                    if (h == IntPtr.Zero) continue;
                    try
                    {
                        StringBuilder sb = new StringBuilder(1024);
                        int n = sb.Capacity;
                        if (QueryFullProcessImageNameW(h, 0, sb, ref n))
                        {
                            string img = sb.ToString(0, n);
                            if (Dentro(img, pasta)) achados.Add(p.ProcessName + " (pid " + p.Id + ")");
                        }
                    }
                    finally { CloseHandle(h); }
                }
                catch (Exception) { }
                finally { p.Dispose(); }
            }
            return achados;
        }

        public static void TestarEscrita(string pasta)
        {
            string teste = Path.Combine(pasta, ".amdlss5_teste_" + Guid.NewGuid().ToString("N").Substring(0, 8));
            try
            {
                File.WriteAllBytes(teste, new byte[] { 1 });
                File.Delete(teste);
            }
            catch (UnauthorizedAccessException) { throw new ErroInstalador(CodigoErro.Permissao, T.F("erro_permissao", pasta)); }
            catch (System.Security.SecurityException) { throw new ErroInstalador(CodigoErro.Permissao, T.F("erro_permissao", pasta)); }
            catch (IOException e) { throw new ErroInstalador(CodigoErro.Permissao, T.F("erro_escrita", pasta, e.Message)); }
        }

        public static Encoding Latin1 { get { return Encoding.GetEncoding(28591); } }

        // Troca atomica: tmp vira fin (File.Replace quando fin existe). Antes apagava fin e depois movia: uma falha no meio
        // deixava a sessao sem acoes.json (o "livro de desfazer").
        public static void TrocarArquivo(string tmp, string fin)
        {
            if (File.Exists(fin)) File.Replace(tmp, fin, null, true);
            else File.Move(tmp, fin);
        }

        // rev-dup (review P2-8): o livro de desfazer de uma sessao (acoes.json) e sempre gravado assim: tmp + troca atomica.
        // Antes repetido na Sessao.Gravar e no Consolidar.
        public static void GravarAcoes(string pastaSessao, object acoes)
        {
            string tmp = Path.Combine(pastaSessao, "acoes.json.tmp");
            File.WriteAllText(tmp, new JavaScriptSerializer().Serialize(acoes), new UTF8Encoding(false));
            TrocarArquivo(tmp, Path.Combine(pastaSessao, "acoes.json"));
        }

        // Tira o livro de desfazer (acoes.json e o .tmp de uma troca interrompida): sem ele a sessao nunca mais e aplicada.
        // Antes repetido no Consolidar e no Desinstalar.
        public static void ApagarAcoes(string pastaSessao)
        {
            foreach (string j in new[] { Path.Combine(pastaSessao, "acoes.json"), Path.Combine(pastaSessao, "acoes.json.tmp") })
                if (File.Exists(j)) File.Delete(j);
        }

        // Copia (sem sobrescrever) e confere o SHA-256 da copia; diferente = IOException com a mensagem de erro dada.
        public static void CopiarConferindo(string origem, string destino, string sha, string erro)
        {
            File.Copy(origem, destino);
            TirarSomenteLeitura(destino);   // review P2 (26/10): o File.Copy leva o somente leitura do pacote para o jogo
            if (Sha256(destino) != sha) throw new IOException(erro);
        }

        // Copia de backup sem o atributo somente leitura (copiado do original pelo File.Copy): senao a pasta de backup nao
        // sai ao desinstalar/consolidar e toda desinstalacao termina "parcial".
        public static void TirarSomenteLeitura(string arquivo)
        {
            FileAttributes at = File.GetAttributes(arquivo);
            if ((at & FileAttributes.ReadOnly) != 0) File.SetAttributes(arquivo, at & ~FileAttributes.ReadOnly);
        }

        // Apaga uma pasta (de backup) mesmo com arquivos somente leitura dentro. Nunca entra em junction/link: o link sai,
        // o destino fica.
        public static void ApagarPasta(string pasta)
        {
            if (!Directory.Exists(pasta)) return;
            if ((File.GetAttributes(pasta) & FileAttributes.ReparsePoint) == 0)
            {
                foreach (string f in Directory.GetFiles(pasta)) TirarSomenteLeitura(f);
                foreach (string d in Directory.GetDirectories(pasta)) ApagarPasta(d);
            }
            Directory.Delete(pasta, (File.GetAttributes(pasta) & FileAttributes.ReparsePoint) == 0);
        }

        // Caminho de um backup citado no acoes.json: so um nome de arquivo dentro da pasta da sessao (null = recusado).
        public static string CaminhoBackup(string pastaSessao, object nome)
        {
            string n = Convert.ToString(nome);
            if (string.IsNullOrEmpty(n) || n.IndexOfAny(new char[] { '\\', '/', ':' }) >= 0 || n.Contains("..") || n.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0) return null;
            string c = Path.GetFullPath(Path.Combine(pastaSessao, n));
            return Dentro(c, pastaSessao) ? c : null;
        }

        // Argumento de linha de comando com aspas (regras do CommandLineToArgvW): barras antes de uma aspa, inclusive a
        // final de "E:\", sao dobradas; senao "E:\" vira E:" e engole o argumento seguinte.
        public static string Argumento(string valor)
        {
            if (valor == null) valor = "";
            StringBuilder sb = new StringBuilder("\"");
            int barras = 0;
            foreach (char c in valor)
            {
                if (c == '\\') { barras++; continue; }
                if (c == '"') { sb.Append('\\', barras * 2 + 1); sb.Append('"'); barras = 0; continue; }
                sb.Append('\\', barras); barras = 0;
                sb.Append(c);
            }
            sb.Append('\\', barras * 2);
            sb.Append('"');
            return sb.ToString();
        }
    }

    // ------------------------------------------------------------------------------------------
    // Pacote (payload): pasta "payload" ao lado do exe.
    //   payload\pacote.json           {"produto","versao","arquivos":{nome:sha256},"reframework":{arquivo,sha256,revisao}}
    //   payload\mod\<arquivos>        o que vai para a pasta do exe do jogo
    //   payload\ini\<ExeSemExt>.ini   (opcional) INI especifico de um jogo, no lugar do INI padrao
    //   payload\reframework\dinput8.dll (opcional) REFramework embutido; sem ele, baixa do GitHub
    // ------------------------------------------------------------------------------------------
    public sealed class Payload
    {
        public string Pasta;
        public string Versao = "?";
        public string Produto = "AMDLSS5";
        public readonly Dictionary<string, string> Arquivos = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        public string RefArquivo, RefSha, RefRevisao;
        // "porjogo" do pacote.json: exe sem extensao (minusculo) -> { caminho relativo com '/' -> SHA-256 }; arquivos em payload\porjogo\<exe>\<relativo>
        public readonly Dictionary<string, Dictionary<string, string>> PorJogo = new Dictionary<string, Dictionary<string, string>>(StringComparer.OrdinalIgnoreCase);
        public Dictionary<string, string> Inis;   // "inis" do pacote.json (nome em payload\ini -> SHA-256); null = pacote sem a lista
        // payload\testados.json ("testados" do pacote.json = SHA-256): jogos testados, {loja, id, exe}; vazio = pacote sem a lista
        public readonly List<string[]> Testados = new List<string[]>();

        // Casa por (loja, id) quando a entrada tem id; sem id, pelo nome do exe.
        public bool EhTestado(string loja, string id, string exe)
        {
            string nomeExe = string.IsNullOrEmpty(exe) ? "" : Path.GetFileName(exe);
            foreach (string[] t in Testados)
            {
                if (t[1].Length > 0) { if (string.Equals(t[0], loja, StringComparison.OrdinalIgnoreCase) && string.Equals(t[1], id, StringComparison.OrdinalIgnoreCase)) return true; }
                else if (t[2].Length > 0 && string.Equals(t[2], nomeExe, StringComparison.OrdinalIgnoreCase)) return true;
            }
            return false;
        }

        public static string PastaPadrao()
        {
            return Path.Combine(Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location), "payload");
        }

        public static Payload Carregar(string pasta)
        {
            string json = Path.Combine(pasta, "pacote.json");
            if (!File.Exists(json)) throw new ErroInstalador(CodigoErro.Faltando, T.F("erro_sem_payload", pasta));
            Payload p = new Payload();
            p.Pasta = Path.GetFullPath(pasta);
            Dictionary<string, object> doc;
            try { doc = (Dictionary<string, object>)new JavaScriptSerializer().DeserializeObject(File.ReadAllText(json, Encoding.UTF8)); }
            catch (Exception e) { throw new ErroInstalador(CodigoErro.Geral, T.F("erro_payload_json", json, e.Message)); }
            object v;
            if (doc.TryGetValue("versao", out v) && v != null) p.Versao = v.ToString();
            if (doc.TryGetValue("produto", out v) && v != null) p.Produto = v.ToString();
            if (!doc.TryGetValue("arquivos", out v) || !(v is Dictionary<string, object>))
                throw new ErroInstalador(CodigoErro.Geral, T.F("erro_payload_json", json, "arquivos"));
            foreach (KeyValuePair<string, object> kv in (Dictionary<string, object>)v)
            {
                if (kv.Key.IndexOfAny(new char[] { '\\', '/', ':' }) >= 0 || kv.Key.Contains(".."))
                    throw new ErroInstalador(CodigoErro.Geral, T.F("erro_payload_json", json, kv.Key));
                p.Arquivos[kv.Key] = Convert.ToString(kv.Value).ToUpperInvariant();
            }
            // 1.1.0: o pacote nao leva INI (o helper publico cria o INI da proxy no primeiro jogo); pacotes antigos com INI continuam aceitos.
            if (!p.Arquivos.ContainsKey(Util.NomeProxy) || !p.Arquivos.ContainsKey(Util.NomeHelper))
                throw new ErroInstalador(CodigoErro.Geral, T.F("erro_payload_json", json, Util.NomeProxy + "/" + Util.NomeHelper));
            // INIs por jogo (payload\ini\<Exe>.ini) com hash: pacotes antigos nao tem "inis" (sem conferencia).
            if (doc.TryGetValue("inis", out v) && v is Dictionary<string, object>)
            {
                p.Inis = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                foreach (KeyValuePair<string, object> kv in (Dictionary<string, object>)v)
                {
                    if (kv.Key.IndexOfAny(new char[] { '\\', '/', ':' }) >= 0 || kv.Key.Contains(".."))
                        throw new ErroInstalador(CodigoErro.Geral, T.F("erro_payload_json", json, kv.Key));
                    p.Inis[kv.Key] = Convert.ToString(kv.Value).ToUpperInvariant();
                }
            }
            // extras por jogo: mesma validacao de nome dos "arquivos"; no relativo o '/' e permitido so como separador
            if (doc.TryGetValue("porjogo", out v) && v is Dictionary<string, object>)
            {
                foreach (KeyValuePair<string, object> jogo in (Dictionary<string, object>)v)
                {
                    if (jogo.Key.Length == 0 || jogo.Key.IndexOfAny(new char[] { '\\', '/', ':' }) >= 0 || jogo.Key.Contains("..") || !(jogo.Value is Dictionary<string, object>))
                        throw new ErroInstalador(CodigoErro.Geral, T.F("erro_payload_json", json, "porjogo/" + jogo.Key));
                    Dictionary<string, string> lista = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                    foreach (KeyValuePair<string, object> kv in (Dictionary<string, object>)jogo.Value)
                    {
                        if (!RelativoValido(kv.Key))
                            throw new ErroInstalador(CodigoErro.Geral, T.F("erro_payload_json", json, "porjogo/" + jogo.Key + "/" + kv.Key));
                        lista[kv.Key] = Convert.ToString(kv.Value).ToUpperInvariant();
                    }
                    p.PorJogo[jogo.Key.ToLowerInvariant()] = lista;
                }
            }
            // lista de jogos testados: arquivo conferido pelo hash; pacote sem a chave = lista vazia
            if (doc.TryGetValue("testados", out v) && v != null)
            {
                string arqT = Path.Combine(p.Pasta, "testados.json");
                if (!File.Exists(arqT) || Util.Sha256(arqT) != Convert.ToString(v).ToUpperInvariant())
                    throw new ErroInstalador(CodigoErro.Geral, T.F("erro_payload_json", json, "testados"));
                try
                {
                    Dictionary<string, object> dt = (Dictionary<string, object>)new JavaScriptSerializer().DeserializeObject(File.ReadAllText(arqT, Encoding.UTF8));
                    object lista;
                    if (dt.TryGetValue("jogos", out lista) && lista is System.Collections.IEnumerable)
                        foreach (object o in (System.Collections.IEnumerable)lista)
                        {
                            Dictionary<string, object> e = o as Dictionary<string, object>;
                            if (e == null) continue;
                            object x;
                            string[] t = new string[3];
                            for (int k = 0; k < 3; k++) t[k] = e.TryGetValue(new[] { "loja", "id", "exe" }[k], out x) && x != null ? Convert.ToString(x).Trim() : "";
                            if (t[1].Length > 0 || t[2].Length > 0) p.Testados.Add(t);
                        }
                }
                catch (Exception) { throw new ErroInstalador(CodigoErro.Geral, T.F("erro_payload_json", arqT, "jogos")); }
            }
            if (doc.TryGetValue("reframework", out v) && v is Dictionary<string, object>)
            {
                Dictionary<string, object> rf = (Dictionary<string, object>)v;
                object a;
                if (rf.TryGetValue("arquivo", out a) && a != null) p.RefArquivo = a.ToString();
                if (rf.TryGetValue("sha256", out a) && a != null) p.RefSha = a.ToString().ToUpperInvariant();
                if (rf.TryGetValue("revisao", out a) && a != null) p.RefRevisao = a.ToString();
            }
            return p;
        }

        static bool RelativoValido(string rel)
        {
            if (string.IsNullOrEmpty(rel) || rel.IndexOfAny(new char[] { '\\', ':' }) >= 0 || rel.Contains("..")) return false;
            foreach (string seg in rel.Split('/')) if (seg.Length == 0 || seg.Trim() != seg) return false;
            return true;
        }

        // Extras do exe (nome com ou sem .exe): caminho relativo (com '/') -> caminho absoluto em payload\porjogo\<exe>\. Vazio se nada.
        public Dictionary<string, string> ExtrasPara(string exe)
        {
            Dictionary<string, string> r = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            Dictionary<string, string> lista;
            if (string.IsNullOrEmpty(exe)) return r;
            string chave = Path.GetFileNameWithoutExtension(exe).ToLowerInvariant();
            if (!PorJogo.TryGetValue(chave, out lista)) return r;
            string baseJogo = Path.Combine(Pasta, "porjogo", chave);
            foreach (KeyValuePair<string, string> kv in lista)
            {
                // pacote-160 (27/10): extra repetido entre jogos fica uma vez so em payload\porjogo\_comum\<SHA>\<nome> (o hash e conferido igual)
                string proprio = Path.GetFullPath(Path.Combine(baseJogo, kv.Key.Replace('/', '\\')));
                string comum = Path.GetFullPath(Path.Combine(Pasta, "porjogo", "_comum", kv.Value, Path.GetFileName(kv.Key.Replace('/', '\\'))));
                r[kv.Key] = !File.Exists(proprio) && File.Exists(comum) ? comum : proprio;
            }
            return r;
        }

        public string CaminhoMod(string nome) { return Path.Combine(Pasta, "mod", nome); }
        public bool Tem(string nome) { return File.Exists(CaminhoMod(nome)); }

        public string ReframeworkEmbutido()
        {
            if (string.IsNullOrEmpty(RefArquivo)) return null;
            string c = Path.GetFullPath(Path.Combine(Pasta, RefArquivo));
            if (!Util.Dentro(c, Pasta) || !File.Exists(c)) return null;
            return c;
        }

        // INI especifico do jogo (payload\ini\<Exe>.ini) ou o padrao do pacote.
        public string IniPara(string exe)
        {
            if (!string.IsNullOrEmpty(exe))
            {
                string c = Path.Combine(Pasta, "ini", Path.GetFileNameWithoutExtension(exe) + ".ini");
                if (File.Exists(c)) return c;
            }
            return CaminhoMod(Util.NomeIni);
        }

        public List<string> FaltandoNvidia()
        {
            List<string> f = new List<string>();
            foreach (string n in Util.ArquivosNvidia) if (Arquivos.ContainsKey(n) && !Tem(n)) f.Add(n);
            return f;
        }

        public long Bytes()
        {
            long t = 0;
            foreach (string n in Arquivos.Keys) if (Tem(n)) t += new FileInfo(CaminhoMod(n)).Length;
            return t;
        }
    }

    // ------------------------------------------------------------------------------------------
    // Deteccao do jogo
    // ------------------------------------------------------------------------------------------
    public sealed class Jogo
    {
        public string Escolhido;     // o que o usuario escolheu (pasta ou exe)
        public string Raiz;          // pasta usada na guarda de "jogo aberto"
        public string Exe;           // exe do jogo (null se nao achou)
        public string PastaMod;      // pasta do exe: onde o mod vai
        public bool ExeX64, ExeImportaVersion;
        public string FfxTipo = "nenhum"; // monolitico | split | outro | nenhum
        public string FfxArquivo, FfxVersao;
        public bool Dx12;
        public bool ReEngine;
        public string ReMotivo;
        public bool TemStorage;
        public int SessoesBackup;
        public string VersaoInstalada;   // null = mod nao instalado por nos; "?" = instalado por versao antiga, sem versao.txt
        public bool ProxyNossoSemBackup;
        public bool VersionDllAlheio;
        public string Dinput8Sha;
        public readonly List<string> Bloqueios = new List<string>();
        public readonly List<string> Avisos = new List<string>();

        public string Nome
        {
            get
            {
                if (Exe == null) return Path.GetFileName(Escolhido.TrimEnd('\\'));
                try
                {
                    FileVersionInfo vi = FileVersionInfo.GetVersionInfo(Exe);
                    if (!string.IsNullOrEmpty(vi.ProductName) && vi.ProductName.Trim().Length > 1) return vi.ProductName.Trim();
                }
                catch (Exception) { }
                return Path.GetFileNameWithoutExtension(Exe);
            }
        }
    }

    public static class Detector
    {
        static readonly string[] PastasIgnoradas = {
            "_storage_", Util.PastaBackup.ToLowerInvariant(), "reframework", "_commonredist", "commonredist", "redist",
            "redistributables", "__installer", "directx", "vcredist", "support", "crashreporter", "easyanticheat",
            "battleye", ".git", "engine", "localization", "movies", "content", "natives", "archive", "r6", "red4ext", "plugins" };
        static readonly string[] ExesIgnorados = { "crash", "unins", "setup", "redist", "launcher", "report", "install", "vc_", "dxsetup", "dotnet", "ue4prereq", "easyanticheat", "be_service", "cefprocess", "helper" };

        public static Jogo Detectar(string escolhido, Payload payload)
        {
            Jogo j = new Jogo();
            j.Escolhido = Path.GetFullPath(escolhido);
            if (File.Exists(j.Escolhido))
            {
                if (!j.Escolhido.EndsWith(".exe", StringComparison.OrdinalIgnoreCase))
                {
                    j.Bloqueios.Add(T.F("det_nao_exe", j.Escolhido));
                    j.Raiz = Path.GetDirectoryName(j.Escolhido);
                    return j;
                }
                j.Exe = j.Escolhido;
                j.Raiz = Path.GetDirectoryName(j.Exe);
            }
            else if (Directory.Exists(j.Escolhido))
            {
                j.Raiz = j.Escolhido;
                j.Exe = MelhorExe(j.Escolhido);
            }
            else
            {
                j.Bloqueios.Add(T.F("det_nao_existe", j.Escolhido));
                j.Raiz = j.Escolhido;
                return j;
            }
            if (j.Exe == null)
            {
                j.Bloqueios.Add(T.F("det_sem_exe", j.Escolhido));
                return j;
            }
            j.PastaMod = Path.GetDirectoryName(j.Exe);
            InfoPe pe = Pe.Ler(j.Exe);
            j.ExeX64 = pe != null && pe.X64;
            j.ExeImportaVersion = pe != null && (pe.ImportaDll("version.dll") || VersionPorDependencia(pe, j.PastaMod));
            if (pe == null || !j.ExeX64) j.Bloqueios.Add(T.F("det_nao_x64", Path.GetFileName(j.Exe)));
            else if (!j.ExeImportaVersion) j.Bloqueios.Add(T.F("det_sem_version", Path.GetFileName(j.Exe)));

            // FSR 3.1 (FidelityFX API): monolitico (amd_fidelityfx_dx12.dll) ou dividido (loader + upscaler)
            string mono = Path.Combine(j.PastaMod, "amd_fidelityfx_dx12.dll");
            string loader = Path.Combine(j.PastaMod, "amd_fidelityfx_loader_dx12.dll");
            string upscaler = Path.Combine(j.PastaMod, "amd_fidelityfx_upscaler_dx12.dll");
            if (File.Exists(mono)) { j.FfxTipo = "monolitico"; j.FfxArquivo = mono; }
            else if (File.Exists(loader) && File.Exists(upscaler)) { j.FfxTipo = "split"; j.FfxArquivo = upscaler; }
            else
            {
                foreach (string f in Directory.GetFiles(j.PastaMod))
                {
                    string n = Path.GetFileName(f).ToLowerInvariant();
                    if (n.Contains("fidelityfx") || n.StartsWith("ffx_fsr") || n.StartsWith("fsr")) { j.FfxTipo = "outro"; j.FfxArquivo = f; break; }
                }
            }
            // pacote-160 (26/10): jogo sem FSR na pasta cujo pacote traz o FSR nos extras dele (payload\porjogo\<exe>: Shadow of the Tomb
            // Raider, onde a nossa libxess troca o XeSS pelo FSR) conta como FSR dividido; as DLLs entram na instalacao com os extras.
            // rdr2 (26/10): o mesmo vale para o jogo com FSR 2 na pasta ("outro": Red Dead Redemption 2, ffx_fsr2_api_x64.dll) cujo
            // pacote traz o loader/upscaler da FidelityFX API: o helper (NeuralFsr2ParaFfx=1 no INI do jogo) leva o FSR 2 a eles.
            if ((j.FfxTipo == "nenhum" || j.FfxTipo == "outro") && payload != null)
            {
                Dictionary<string, string> extras = payload.ExtrasPara(j.Exe);
                string upPacote;
                if (extras.TryGetValue("amd_fidelityfx_upscaler_dx12.dll", out upPacote) && extras.ContainsKey("amd_fidelityfx_loader_dx12.dll"))
                { j.FfxTipo = "split"; j.FfxArquivo = upPacote; }
            }
            if (j.FfxArquivo != null)
            {
                try
                {
                    FileVersionInfo vi = FileVersionInfo.GetVersionInfo(j.FfxArquivo);
                    j.FfxVersao = string.IsNullOrEmpty(vi.ProductVersion) ? vi.FileVersion : vi.ProductVersion;
                }
                catch (Exception) { }
            }
            if (j.FfxTipo == "nenhum") j.Avisos.Add(T.S("det_sem_fsr"));
            else if (j.FfxTipo == "outro") j.Avisos.Add(T.F("det_fsr_outro", Path.GetFileName(j.FfxArquivo)));

            j.Dx12 = (pe != null && pe.ImportaDll("d3d12.dll")) || j.FfxTipo == "monolitico" || j.FfxTipo == "split"
                || File.Exists(Path.Combine(j.PastaMod, "D3D12", "D3D12Core.dll"));
            if (!j.Dx12) j.Avisos.Add(T.S("det_sem_dx12"));

            // RE Engine (Capcom): reN.exe ou re_chunk_000.pak -> REFramework (dinput8.dll) e requisito
            string exeNome = Path.GetFileName(j.Exe);
            if (Regex.IsMatch(exeNome, @"^re\d+\.exe$", RegexOptions.IgnoreCase)) { j.ReEngine = true; j.ReMotivo = exeNome; }
            else if (File.Exists(Path.Combine(j.PastaMod, "re_chunk_000.pak"))) { j.ReEngine = true; j.ReMotivo = "re_chunk_000.pak"; }
            j.TemStorage = Directory.Exists(Path.Combine(j.PastaMod, "_storage_"));

            string bk = Path.Combine(j.PastaMod, Util.PastaBackup);
            j.SessoesBackup = Sessoes.Validas(bk, false).Count;   // pasta sem acoes.json nao desfaz nada: nao conta
            if (j.SessoesBackup > 0)
            {
                List<string> sessoesOk = Sessoes.Validas(bk, false);
                string arqVersao = Path.Combine(sessoesOk[sessoesOk.Count - 1], "versao.txt");
                j.VersaoInstalada = "?";
                try { if (File.Exists(arqVersao)) { string v = File.ReadAllText(arqVersao, Encoding.UTF8).Trim(); if (v.Length > 0) j.VersaoInstalada = v; } }
                catch (Exception) { }
            }
            string ver = Path.Combine(j.PastaMod, Util.NomeProxy);
            if (File.Exists(ver) && payload != null)
            {
                string h = Util.Sha256(ver);
                string nosso;
                payload.Arquivos.TryGetValue(Util.NomeProxy, out nosso);
                if (h == nosso) { if (j.SessoesBackup == 0) j.ProxyNossoSemBackup = true; }
                else if (j.SessoesBackup == 0) { j.VersionDllAlheio = true; j.Avisos.Add(T.F("det_version_alheio", Util.Curto(h))); }
            }
            string di = Path.Combine(j.PastaMod, "dinput8.dll");
            if (File.Exists(di)) j.Dinput8Sha = Util.Sha256(di);
            return j;
        }

        // version.dll carregada por uma DLL do proprio jogo que o exe importa (ex.: Witcher 3 next-gen: xaudio2_9redist.dll,
        // GFSDK_Aftermath_Lib.x64.dll). So a pasta do exe: as DLLs do sistema nao contam.
        static bool VersionPorDependencia(InfoPe pe, string pasta)
        {
            foreach (string s in pe.Importa)
            {
                string f = Path.Combine(pasta, s);
                if (!File.Exists(f)) continue;
                InfoPe d = Pe.Ler(f);
                if (d != null && d.X64 && d.ImportaDll("version.dll")) return true;
            }
            return false;
        }

        // Procura o exe do jogo ate 3 niveis abaixo (ex.: Cyberpunk em bin\x64). Pontua: x64 + importa
        // version.dll (onde o proxy se hospeda) + FSR ao lado + d3d12 + tamanho.
        public static string MelhorExe(string pasta)
        {
            List<KeyValuePair<string, int>> exes = new List<KeyValuePair<string, int>>();
            Coletar(pasta, 0, exes);
            string melhor = null;
            long melhorPontos = long.MinValue;
            foreach (KeyValuePair<string, int> kv in exes)
            {
                string nome = Path.GetFileName(kv.Key).ToLowerInvariant();
                bool ignorar = false;
                foreach (string s in ExesIgnorados) if (nome.Contains(s)) { ignorar = true; break; }
                if (ignorar) continue;
                InfoPe pe = Pe.Ler(kv.Key);
                if (pe == null || !pe.X64 || pe.EhDll) continue;
                long pontos = 0;
                if (pe.ImportaDll("version.dll")) pontos += 100;
                string dir = Path.GetDirectoryName(kv.Key);
                if (File.Exists(Path.Combine(dir, "amd_fidelityfx_dx12.dll")) || File.Exists(Path.Combine(dir, "amd_fidelityfx_upscaler_dx12.dll"))) pontos += 50;
                if (pe.ImportaDll("d3d12.dll") || pe.ImportaDll("dxgi.dll")) pontos += 10;
                if (File.Exists(Path.Combine(dir, "re_chunk_000.pak"))) pontos += 10;
                long tam = 0;
                try { tam = new FileInfo(kv.Key).Length; } catch (Exception) { }
                pontos += Math.Min(tam / (50L << 20), 10);
                pontos -= kv.Value * 2;
                if (pontos > melhorPontos) { melhorPontos = pontos; melhor = kv.Key; }
            }
            return melhor;
        }

        // Primeiro exe de jogo de 32 bits (nao ignorado) na pasta, ou null: a pasta tem jogo, mas o mod nao serve para ele.
        public static string Exe32Bits(string pasta)
        {
            List<KeyValuePair<string, int>> exes = new List<KeyValuePair<string, int>>();
            Coletar(pasta, 0, exes);
            foreach (KeyValuePair<string, int> kv in exes)
            {
                string nome = Path.GetFileName(kv.Key).ToLowerInvariant();
                bool ignorar = false;
                foreach (string s in ExesIgnorados) if (nome.Contains(s)) { ignorar = true; break; }
                if (ignorar) continue;
                InfoPe pe = Pe.Ler(kv.Key);
                if (pe != null && !pe.X64 && !pe.EhDll) return kv.Key;
            }
            return null;
        }

        static void Coletar(string dir, int prof, List<KeyValuePair<string, int>> saida)
        {
            if (prof > 3) return;
            try { foreach (string f in Directory.GetFiles(dir, "*.exe")) saida.Add(new KeyValuePair<string, int>(f, prof)); }
            catch (Exception) { }
            string[] subs;
            try { subs = Directory.GetDirectories(dir); } catch (Exception) { return; }
            foreach (string d in subs)
            {
                string n = Path.GetFileName(d).ToLowerInvariant();
                if (Array.IndexOf(PastasIgnoradas, n) >= 0) continue;
                try { if ((File.GetAttributes(d) & FileAttributes.ReparsePoint) != 0) continue; } catch (Exception) { continue; }
                Coletar(d, prof + 1, saida);
            }
        }
    }

    // ------------------------------------------------------------------------------------------
    // REFramework (praydog, MIT): embutido no pacote ou baixado do release oficial (nightly).
    // ------------------------------------------------------------------------------------------
    public static class Reframework
    {
        public const string Api = "https://api.github.com/repos/praydog/REFramework-nightly/releases/latest";
        const string Prefixo = "https://github.com/praydog/REFramework-nightly/releases/download/";

        public static bool DllValida(string caminho)
        {
            InfoPe pe = Pe.Ler(caminho);
            return pe != null && pe.X64 && pe.EhDll && pe.ExportaNome("DirectInput8Create");
        }

        public static string Obter(Payload p, bool permitirDownload, IRelato r)
        {
            string emb = p.ReframeworkEmbutido();
            if (emb != null)
            {
                if (!string.IsNullOrEmpty(p.RefSha) && Util.Sha256(emb) != p.RefSha)
                    throw new ErroInstalador(CodigoErro.Geral, T.F("erro_hash", "reframework/dinput8.dll"));
                if (!DllValida(emb)) throw new ErroInstalador(CodigoErro.Geral, T.F("erro_ref_invalido", emb));
                r.Linha(T.F("ref_embutido", p.RefRevisao ?? "?"));
                return emb;
            }
            if (!permitirDownload) throw new ErroInstalador(CodigoErro.Faltando, T.S("erro_ref_sem"));
            return Baixar(r);
        }

        public static string Baixar(IRelato r)
        {
            ServicePointManager.SecurityProtocol = (SecurityProtocolType)3072; // TLS 1.2
            r.Linha(T.F("ref_consultando", Api));
            string json;
            using (WebClient wc = new WebClient())
            {
                wc.Headers[HttpRequestHeader.UserAgent] = "AMDLSS5-Instalador";
                wc.Encoding = Encoding.UTF8;
                try { json = wc.DownloadString(Api); }
                catch (WebException e) { throw new ErroInstalador(CodigoErro.Geral, T.F("erro_ref_rede", e.Message)); }
            }
            Dictionary<string, object> rel = (Dictionary<string, object>)new JavaScriptSerializer().DeserializeObject(json);
            string tag = rel.ContainsKey("tag_name") ? Convert.ToString(rel["tag_name"]) : "?";
            string url = null;
            long tam = 0;
            object assets;
            if (rel.TryGetValue("assets", out assets) && assets is object[])
            {
                foreach (object o in (object[])assets)
                {
                    Dictionary<string, object> a = o as Dictionary<string, object>;
                    if (a == null || Convert.ToString(a["name"]) != "REFramework.zip") continue;
                    url = Convert.ToString(a["browser_download_url"]);
                    tam = Convert.ToInt64(a["size"]);
                }
            }
            if (url == null) throw new ErroInstalador(CodigoErro.Geral, T.F("erro_ref_sem_zip", tag));
            if (!url.StartsWith(Prefixo, StringComparison.Ordinal)) throw new ErroInstalador(CodigoErro.Geral, T.F("erro_ref_url", url));
            string pasta = Path.Combine(Util.TempInstalador(), "reframework", Regex.Replace(tag, @"[^A-Za-z0-9._-]", "_"));
            Directory.CreateDirectory(pasta);
            string zip = Path.Combine(pasta, "REFramework.zip");
            r.Linha(T.F("ref_baixando", tag, Util.Mb(tam), url));
            HttpWebRequest req = (HttpWebRequest)WebRequest.Create(url);
            req.UserAgent = "AMDLSS5-Instalador";
            req.AllowAutoRedirect = true;
            try
            {
                using (WebResponse resp = req.GetResponse())
                using (Stream s = resp.GetResponseStream())
                using (FileStream fs = File.Create(zip))
                {
                    byte[] buf = new byte[1 << 16];
                    long lido = 0;
                    int n, ultimo = -1;
                    while ((n = s.Read(buf, 0, buf.Length)) > 0)
                    {
                        fs.Write(buf, 0, n);
                        lido += n;
                        int pct = tam > 0 ? (int)(lido * 100 / tam) : 0;
                        if (pct != ultimo) { r.Progresso(pct); ultimo = pct; }
                    }
                }
            }
            catch (WebException e) { throw new ErroInstalador(CodigoErro.Geral, T.F("erro_ref_rede", e.Message)); }
            if (new FileInfo(zip).Length != tam) throw new ErroInstalador(CodigoErro.Geral, T.S("erro_ref_incompleto"));
            string dll = Path.Combine(pasta, "dinput8.dll");
            using (ZipArchive z = ZipFile.OpenRead(zip))
            {
                ZipArchiveEntry e = z.GetEntry("dinput8.dll");
                if (e == null) throw new ErroInstalador(CodigoErro.Geral, T.S("erro_ref_sem_dll"));
                e.ExtractToFile(dll, true);
            }
            if (!DllValida(dll)) throw new ErroInstalador(CodigoErro.Geral, T.F("erro_ref_invalido", dll));
            r.Linha(T.F("ref_baixado", tag, Util.Curto(Util.Sha256(dll))));
            return dll;
        }
    }

    // ------------------------------------------------------------------------------------------
    // Instalacao e desinstalacao
    // ------------------------------------------------------------------------------------------
    public sealed class OpcoesInstalacao
    {
        public bool InstalarReframework;
        public bool PermitirDownload; // REFramework do GitHub so com pedido explicito (o pacote ja traz o dinput8.dll)
        public string Idioma; // pt | en | es -> MenuLanguage do INI novo
        public readonly Dictionary<string, string> Fornecidos = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
    }

    sealed class Sessao : IRelato
    {
        public string Pasta;
        public string PastaMod;
        public List<Dictionary<string, object>> Acoes = new List<Dictionary<string, object>>();
        StreamWriter log;
        IRelato fora;

        public Sessao(string pastaMod, string pasta, IRelato fora, string nomeLog)
        {
            PastaMod = pastaMod; Pasta = pasta; this.fora = fora;
            log = new StreamWriter(Path.Combine(pasta, nomeLog), true, new UTF8Encoding(false));
            log.AutoFlush = true;
        }

        public void Linha(string texto)
        {
            if (log != null) log.WriteLine(DateTime.Now.ToString("HH:mm:ss") + "  " + texto);
            fora.Linha(texto);
        }
        public void Progresso(int p) { fora.Progresso(p); }
        public void Fechar() { if (log != null) { log.Dispose(); log = null; } }

        public void Registrar(Dictionary<string, object> acao)
        {
            Acoes.Add(acao);
            Gravar();
        }

        public void Gravar() { Util.GravarAcoes(Pasta, Acoes); }
    }

    // Pastas de sessao em AMDLSS5_backup. Valida = tem acoes.json (ou so o acoes.json.tmp de uma troca interrompida, que e
    // recuperado). Pasta sem nenhum dos dois nao desfaz nada (sobra de uma limpeza que falhou): fica fora da lista e,
    // com limpar, e apagada (se nao der, fica para a proxima vez; nunca trava a consolidacao nem a desinstalacao).
    static class Sessoes
    {
        public static List<string> Validas(string bkRaiz, bool limpar)
        {
            List<string> validas = new List<string>();
            if (!Directory.Exists(bkRaiz)) return validas;
            List<string> todas = new List<string>(Directory.GetDirectories(bkRaiz));
            todas.Sort(StringComparer.Ordinal);
            foreach (string d in todas)
            {
                string json = Path.Combine(d, "acoes.json"), tmp = json + ".tmp";
                if (!File.Exists(json) && File.Exists(tmp))
                {
                    try { File.Move(tmp, json); } catch (Exception) { }
                }
                if (File.Exists(json) || File.Exists(tmp)) { validas.Add(d); continue; }
                if (limpar) { try { Util.ApagarPasta(d); } catch (Exception) { } }
            }
            return validas;
        }

        public static string Json(string sessao)
        {
            string json = Path.Combine(sessao, "acoes.json");
            return File.Exists(json) ? json : json + ".tmp";
        }

        public static List<Dictionary<string, object>> Ler(string sessao)
        {
            List<Dictionary<string, object>> acoes = new List<Dictionary<string, object>>();
            object[] lista = new JavaScriptSerializer().DeserializeObject(File.ReadAllText(Json(sessao), Encoding.UTF8)) as object[];
            if (lista != null) foreach (object o in lista) { Dictionary<string, object> a = o as Dictionary<string, object>; if (a != null && a.ContainsKey("acao") && a.ContainsKey("caminho")) acoes.Add(a); }
            return acoes;
        }

        // Ha algo a desfazer nesta pasta do jogo (uma sessao valida)?
        public static bool Resta(string pastaMod)
        {
            return pastaMod != null && Validas(Path.Combine(pastaMod, Util.PastaBackup), false).Count > 0;
        }

        // Primeira acao (a da sessao mais velha) de cada caminho: diz se o arquivo existia antes do mod.
        public static Dictionary<string, string> PrimeiraAcao(string bkRaiz)
        {
            Dictionary<string, string> r = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            try
            {
                foreach (string d in Validas(bkRaiz, false))
                    foreach (Dictionary<string, object> a in Ler(d))
                    {
                        string rel = Convert.ToString(a["caminho"]);
                        if (!r.ContainsKey(rel)) r[rel] = Convert.ToString(a["acao"]);
                    }
            }
            catch (Exception) { }
            return r;
        }
    }

    public static class Instalacao
    {
        public static string Instalar(Jogo j, Payload p, OpcoesInstalacao o, IRelato r)
        {
            if (j.Bloqueios.Count > 0) throw new ErroInstalador(CodigoErro.Bloqueado, string.Join(Environment.NewLine, j.Bloqueios.ToArray()));
            GuardaJogoAberto(j);

            // alvos: pasta do exe e, se existir, _storage_ (o RE carrega o mod de la)
            List<string> alvos = new List<string>();
            alvos.Add(j.PastaMod);
            string storage = Path.Combine(j.PastaMod, "_storage_");
            if (Directory.Exists(storage)) alvos.Add(storage);
            foreach (string a in alvos) Util.TestarEscrita(a);

            // arquivos: do pacote ou fornecidos pelo usuario (os da NVIDIA)
            Dictionary<string, string> origem = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            foreach (string nome in p.Arquivos.Keys)
            {
                if (nome.Equals(Util.NomeIni, StringComparison.OrdinalIgnoreCase)) continue;
                if (p.Tem(nome)) { origem[nome] = p.CaminhoMod(nome); continue; }
                string forn;
                if (Array.IndexOf(Util.ArquivosNvidia, nome) >= 0 && o.Fornecidos.TryGetValue(nome, out forn) && File.Exists(forn)) { origem[nome] = forn; continue; }
                throw new ErroInstalador(CodigoErro.Faltando, T.F(nome == Util.NomePesos ? "erro_sem_pesos" : nome == Util.NomeNvngx ? "erro_sem_nvngx" : "erro_payload_falta", nome));
            }
            // INI so em pacote antigo (com dlssnr_on_amd.ini no manifesto); o 1.1.0 nao leva INI.
            string iniOrigem = p.Arquivos.ContainsKey(Util.NomeIni) ? p.IniPara(j.Exe) : null;
            if (iniOrigem != null && !File.Exists(iniOrigem)) throw new ErroInstalador(CodigoErro.Faltando, T.F("erro_payload_falta", Util.NomeIni));
            if (iniOrigem != null)
            {
                // o INI do pacote tambem e conferido: o padrao pelo "arquivos", o de um jogo (payload\ini) pelo "inis"
                string esperadoIni = null;
                if (Util.Dentro(iniOrigem, Path.Combine(p.Pasta, "ini")))
                {
                    if (p.Inis != null && !p.Inis.TryGetValue(Path.GetFileName(iniOrigem), out esperadoIni))
                        throw new ErroInstalador(CodigoErro.Geral, T.F("erro_hash", "ini/" + Path.GetFileName(iniOrigem)));
                }
                else esperadoIni = p.Arquivos[Util.NomeIni];
                if (esperadoIni != null && Util.Sha256(iniOrigem) != esperadoIni)
                    throw new ErroInstalador(CodigoErro.Geral, T.F("erro_hash", Util.Dentro(iniOrigem, Path.Combine(p.Pasta, "ini")) ? "ini/" + Path.GetFileName(iniOrigem) : Util.NomeIni));
            }

            // extras do jogo (payload\porjogo\<exe>): so entram na pasta do exe; o hash e conferido antes de tocar no jogo
            string chaveExe = Path.GetFileNameWithoutExtension(j.Exe).ToLowerInvariant();
            Dictionary<string, string> extras = p.ExtrasPara(j.Exe);
            Dictionary<string, string> extrasSha = null;
            if (extras.Count > 0) extrasSha = p.PorJogo[chaveExe];
            string pastaExtras = Path.Combine(p.Pasta, "porjogo");
            foreach (KeyValuePair<string, string> kv in extras)
            {
                string nomeExtra = "porjogo/" + chaveExe + "/" + kv.Key;
                if (!Util.Dentro(kv.Value, pastaExtras) || !File.Exists(kv.Value)) throw new ErroInstalador(CodigoErro.Faltando, T.F("erro_payload_falta", nomeExtra));
                if (Util.Sha256(kv.Value) != extrasSha[kv.Key]) throw new ErroInstalador(CodigoErro.Geral, T.F("erro_hash", nomeExtra));
            }

            // espaco
            long bytes = 0;
            foreach (string c in origem.Values) bytes += new FileInfo(c).Length;
            foreach (string c in extras.Values) bytes += new FileInfo(c).Length;
            long livre = new DriveInfo(Path.GetPathRoot(j.PastaMod)).AvailableFreeSpace;
            long precisa = bytes * alvos.Count * 2 + (200L << 20);
            if (livre < precisa) throw new ErroInstalador(CodigoErro.Geral, T.F("erro_espaco", Util.Mb(livre), Util.Mb(precisa)));

            // conferencia do pacote (hash) antes de tocar no jogo
            r.Linha(T.F("conferindo_pacote", p.Versao, origem.Count));
            int i = 0;
            foreach (KeyValuePair<string, string> kv in origem)
            {
                string h = Util.Sha256(kv.Value);
                string esperado = p.Arquivos[kv.Key];
                if (h != esperado)
                {
                    bool doUsuario = !Util.Dentro(kv.Value, p.Pasta);
                    if (!doUsuario) throw new ErroInstalador(CodigoErro.Geral, T.F("erro_hash", kv.Key));
                    r.Linha(T.F("aviso_hash_usuario", kv.Key, Util.Curto(h), Util.Curto(esperado)));
                    p.Arquivos[kv.Key] = h; // instala o do usuario; o registro guarda o hash real
                }
                if (kv.Key.EndsWith(".dll", StringComparison.OrdinalIgnoreCase))
                {
                    InfoPe pe = Pe.Ler(kv.Value);
                    if (pe == null || !pe.X64 || !pe.EhDll) throw new ErroInstalador(CodigoErro.Geral, T.F("erro_dll_invalida", kv.Key, kv.Value));
                }
                r.Progresso(++i * 30 / origem.Count);
            }

            // REFramework antes de tocar no jogo (um download que falha nao deixa nada pela metade)
            string refDll = null;
            if (o.InstalarReframework)
            {
                string di = Path.Combine(j.PastaMod, "dinput8.dll");
                if (File.Exists(di) && !Reframework.DllValida(di)) r.Linha(T.S("ref_alheio_mantido"));
                else refDll = Reframework.Obter(p, o.PermitirDownload, r);
            }

            string bkRaiz = Path.Combine(j.PastaMod, Util.PastaBackup);
            // INI que o mod criou numa instalacao anterior (1a acao = ini_novo/criado_pelo_mod): e trocado pelo do pacote
            // (so as chaves do jogador ficam); INI que ja existia antes do mod continua so recebendo as chaves que faltam.
            Dictionary<string, string> primeira = Sessoes.PrimeiraAcao(bkRaiz);
            string nomeSessao = DateTime.Now.ToString("yyyyMMdd-HHmmss");
            string pastaSessao = Path.Combine(bkRaiz, nomeSessao);
            for (int k = 2; Directory.Exists(pastaSessao); k++) pastaSessao = Path.Combine(bkRaiz, nomeSessao + "-" + k);
            Directory.CreateDirectory(pastaSessao);
            GravarVersao(pastaSessao, p.Versao);
            Sessao s = new Sessao(j.PastaMod, pastaSessao, r, "instalacao.log");
            try
            {
                s.Linha(T.F("log_cabecalho", p.Produto, p.Versao, j.Exe));
                s.Gravar();
                int total = origem.Count * alvos.Count + alvos.Count + 1, feito = 0;
                foreach (string alvo in alvos)
                {
                    s.Linha(T.F("instalando_em", alvo));
                    foreach (KeyValuePair<string, string> kv in origem)
                    {
                        CopiarComBackup(s, kv.Value, Path.Combine(alvo, kv.Key), p.Arquivos[kv.Key]);
                        s.Progresso(30 + ++feito * 70 / total);
                    }
                    if (iniOrigem != null)
                    {
                        string iniDestino = Path.Combine(alvo, Util.NomeIni), tipo1;
                        bool doMod = primeira.TryGetValue(Rel(s, iniDestino), out tipo1) && (tipo1 == "ini_novo" || tipo1 == "criado_pelo_mod");
                        InstalarIni(s, iniOrigem, iniDestino, o.Idioma, doMod);
                    }
                    foreach (string n in Util.ArquivosDoMod)   // 1.3.0: com INI no pacote, os outros arquivos que o mod cria tambem
                        if (iniOrigem == null || !n.Equals(Util.NomeIni, StringComparison.OrdinalIgnoreCase)) GuardarArquivoDoMod(s, Path.Combine(alvo, n));
                    s.Progresso(30 + ++feito * 70 / total);
                }
                foreach (KeyValuePair<string, string> kv in extras)
                {
                    string destinoExtra = Path.GetFullPath(Path.Combine(j.PastaMod, kv.Key.Replace('/', '\\')));
                    if (!Util.Dentro(destinoExtra, j.PastaMod)) throw new ErroInstalador(CodigoErro.Geral, T.F("erro_payload_json", "porjogo", kv.Key));
                    CriarPastas(s, j.PastaMod, Path.GetDirectoryName(destinoExtra));
                    // rdr2 (26/10): DLL da FidelityFX API (amd_fidelityfx_*.dll) que o jogador ja tem numa versao MAIS NOVA que a do
                    // pacote fica como esta (nada registrado: a desinstalacao nao mexe nela); versao igual/mais velha/desconhecida = troca com backup
                    string maisNova = ExtraFfxMaisNovo(destinoExtra, kv.Value);
                    if (maisNova != null) { s.Linha(T.F("arq_ffx_mantido", Rel(s, destinoExtra), maisNova)); continue; }
                    CopiarComBackup(s, kv.Value, destinoExtra, extrasSha[kv.Key]);
                }
                if (refDll != null) InstalarReframework(s, refDll, Path.Combine(j.PastaMod, "dinput8.dll"));
                s.Progresso(100);
                s.Linha(T.F("instalado_ok", pastaSessao));
                if (!j.TemStorage && j.ReEngine) s.Linha(T.S("nota_storage"));
            }
            catch (Exception e)
            {
                s.Linha(T.F("falhou_desfazendo", e.Message));
                bool limpo = false;
                try { limpo = Desfazer(s.Acoes, j.PastaMod, pastaSessao, s); }
                catch (Exception e2) { s.Linha(T.F("desfazer_falhou", e2.Message)); }
                s.Fechar();
                if (limpo)
                {
                    try
                    {
                        // o log desta tentativa vai para %TEMP% antes de a sessao (vazia de efeito) sumir
                        File.Copy(Path.Combine(pastaSessao, "instalacao.log"), Path.Combine(Util.TempInstalador(), "instalacao-falhou-" + Path.GetFileName(pastaSessao) + ".log"), true);
                        Util.ApagarPasta(pastaSessao);
                        if (Directory.GetFileSystemEntries(bkRaiz).Length == 0) Directory.Delete(bkRaiz);
                    }
                    catch (Exception) { }
                }
                ErroInstalador ei = e as ErroInstalador;
                if (ei != null) throw new ErroInstalador(ei.Codigo, ei.Message + Environment.NewLine + T.S(limpo ? "desfeito_ok" : "desfeito_parcial"));
                if (e is UnauthorizedAccessException) throw new ErroInstalador(CodigoErro.Permissao, T.F("erro_permissao", j.PastaMod));
                throw new ErroInstalador(CodigoErro.Geral, e.Message + Environment.NewLine + T.S(limpo ? "desfeito_ok" : "desfeito_parcial"));
            }
            finally { s.Fechar(); }
            string final = Consolidar(bkRaiz, pastaSessao, r);
            if (!string.Equals(final, pastaSessao, StringComparison.OrdinalIgnoreCase)) GravarVersao(final, p.Versao);   // a sessao mais velha sobrevive
            return final;
        }

        // versao do pacote instalado, para a biblioteca mostrar "instalada vX / atualizar" (separado do acoes.json)
        static void GravarVersao(string pastaSessao, string versao)
        {
            try { File.WriteAllText(Path.Combine(pastaSessao, "versao.txt"), versao ?? "?", new UTF8Encoding(false)); }
            catch (Exception) { }
        }

        // 1.5.2: instalar por cima de uma versao anterior nao empilha backup. As sessoes de AMDLSS5_backup viram uma so, a
        // mais velha (a que guarda os arquivos originais do jogo): um arquivo que uma sessao anterior ja tinha posto so
        // atualiza o hash esperado, e o backup da versao velha do mod nao e guardado. Desinstalar continua devolvendo a
        // pasta como era antes da 1a instalacao. Pasta sem acoes.json (sobra de uma limpeza que falhou) nao desfaz nada:
        // fica fora e e apagada se der; nunca desliga a consolidacao.
        static string Consolidar(string bkRaiz, string nova, IRelato r)
        {
            try
            {
                List<string> sessoes = Sessoes.Validas(bkRaiz, true);
                if (sessoes.Count < 2) return nova;
                List<List<Dictionary<string, object>>> listas = new List<List<Dictionary<string, object>>>();
                foreach (string d in sessoes) listas.Add(Sessoes.Ler(d));
                string baseDir = sessoes[0];
                for (int k = 1; k < sessoes.Count; k++)
                    foreach (Dictionary<string, object> a in listas[k]) Juntar(listas[0], a, sessoes[k], baseDir);
                // a base primeiro (tmp + troca atomica); depois tira o acoes.json das outras (sem ele uma sessao nunca e aplicada)
                Util.GravarAcoes(baseDir, listas[0]);
                for (int k = 1; k < sessoes.Count; k++) Util.ApagarAcoes(sessoes[k]);
                long liberado = 0;
                for (int k = 1; k < sessoes.Count; k++)
                {
                    try
                    {
                        string log = Path.Combine(sessoes[k], "instalacao.log");
                        if (File.Exists(log)) File.AppendAllText(Path.Combine(baseDir, "instalacao.log"), Environment.NewLine + File.ReadAllText(log, Encoding.UTF8), new UTF8Encoding(false));
                        foreach (string f in Directory.GetFiles(sessoes[k], "*", SearchOption.AllDirectories)) liberado += new FileInfo(f).Length;
                        Util.ApagarPasta(sessoes[k]);
                    }
                    catch (Exception) { }   // sem acoes.json ela ja nao conta; a proxima instalacao tenta apagar de novo
                }
                r.Linha(T.F("consolidado", sessoes.Count - 1, Util.Mb(liberado), baseDir));
                return baseDir;
            }
            catch (Exception e)
            {
                r.Linha(T.F("consolidar_falhou", e.Message));
                return nova;
            }
        }

        // Junta a acao a (de uma sessao mais nova, pasta dirA) na lista da sessao base. Sem regra especial, a acao vai
        // para o fim da base com o seu backup: desfeita antes das que ja estavam la, como se fosse uma sessao a parte.
        static void Juntar(List<Dictionary<string, object>> base_, Dictionary<string, object> a, string dirA, string baseDir)
        {
            string rel = Convert.ToString(a["caminho"]), tipo = Convert.ToString(a["acao"]);
            Dictionary<string, object> b = null;
            for (int i = base_.Count - 1; i >= 0 && b == null; i--)
                if (string.Equals(Convert.ToString(base_[i]["caminho"]), rel, StringComparison.OrdinalIgnoreCase)) b = base_[i];
            string tb = b == null ? null : Convert.ToString(b["acao"]);
            bool doMod = tb == "ini_novo" || tb == "criado_pelo_mod" || tb == "ini_mesclado" || tb == "preservado" || tb == "ini_trocado";
            switch (tipo)
            {
                case "igual":
                    if (b != null) return;
                    break;
                case "pasta_criada":
                    if (b != null) return;   // a base ja sabe apagar a pasta
                    break;
                case "trocado":
                case "acrescentado":
                    // o arquivo ainda era o que a base pos (ou o jogador o apagou): o original continua no backup da base.
                    // P0-4 (19/10): se ele foi trocado por OUTRO arquivo depois (version.dll de outro mod), o backup desse
                    // arquivo nao pode sumir: a acao vai para o fim da base com a copia (desfeita antes; a base o mantem).
                    if ((tb == "acrescentado" || tb == "trocado")
                        && (tipo == "acrescentado" || string.Equals(Convert.ToString(a["anterior"]), b.ContainsKey("novo") ? Convert.ToString(b["novo"]) : null, StringComparison.OrdinalIgnoreCase)))
                    {
                        b["novo"] = a["novo"];
                        return;
                    }
                    // a base o achou igual (original do jogo = o do mod antigo): o original e o "anterior" desta troca
                    if (tipo == "trocado" && tb == "igual")
                    {
                        b["acao"] = "trocado"; b["backup"] = CopiarBackup(a, dirA, baseDir); b["anterior"] = a["anterior"]; b["novo"] = a["novo"];
                        return;
                    }
                    break;
                case "ini_mesclado":
                case "ini_trocado":
                case "preservado":
                case "ini_novo":
                case "criado_pelo_mod":
                    // INI/.dat do mod: a base ja sabe devolver o original (ou apagar o que o mod criou)
                    if (doMod) return;
                    break;
            }
            if (a.ContainsKey("backup")) a["backup"] = CopiarBackup(a, dirA, baseDir);
            base_.Add(a);
        }

        static string CopiarBackup(Dictionary<string, object> a, string dirA, string baseDir)
        {
            string nome = Convert.ToString(a["backup"]);
            string origem = Util.CaminhoBackup(dirA, nome);
            if (origem == null || !File.Exists(origem)) throw new IOException(T.F("des_sem_backup", Convert.ToString(a["caminho"])));
            string destino = Path.Combine(baseDir, nome);
            if (File.Exists(destino)) { nome = "c" + Guid.NewGuid().ToString("N").Substring(0, 8) + "-" + nome; destino = Path.Combine(baseDir, nome); }
            File.Copy(origem, destino);
            Util.TirarSomenteLeitura(destino);
            return nome;
        }

        public static void GuardaJogoAberto(Jogo j)
        {
            List<string> abertos = Util.ProcessosNaPasta(j.Raiz);
            if (j.PastaMod != null && !Util.Dentro(j.PastaMod, j.Raiz) && !string.Equals(Path.GetFullPath(j.PastaMod).TrimEnd('\\'), Path.GetFullPath(j.Raiz).TrimEnd('\\'), StringComparison.OrdinalIgnoreCase)) abertos.AddRange(Util.ProcessosNaPasta(j.PastaMod));
            if (abertos.Count > 0) throw new ErroInstalador(CodigoErro.JogoAberto, T.F("erro_jogo_aberto", string.Join(", ", abertos.ToArray())));
        }

        static string Rel(Sessao s, string caminho)
        {
            string b = s.PastaMod.TrimEnd('\\') + "\\";
            return caminho.StartsWith(b, StringComparison.OrdinalIgnoreCase) ? caminho.Substring(b.Length) : caminho;
        }

        static Dictionary<string, object> Acao(string tipo, string rel)
        {
            Dictionary<string, object> a = new Dictionary<string, object>();
            a["acao"] = tipo;
            a["caminho"] = rel;
            return a;
        }

        // rev-dup (review P2-8): copia o arquivo atual (SHA-256 'atual') para a pasta da sessao como <prefixo>-xxxxxxxx-<nome>,
        // sem somente leitura, e confere a copia. Devolve o nome do backup. Antes repetido em 3 lugares (arquivo, mod, ini).
        static string GuardarBackup(Sessao s, string destino, string prefixo, string rel, string atual)
        {
            string nomeBk = prefixo + "-" + Guid.NewGuid().ToString("N").Substring(0, 8) + "-" + Path.GetFileName(destino);
            string bk = Path.Combine(s.Pasta, nomeBk);
            File.Copy(destino, bk);
            Util.TirarSomenteLeitura(bk);
            if (Util.Sha256(bk) != atual) throw new IOException(T.F("erro_backup", rel));
            return nomeBk;
        }

        // Cria as pastas que faltam entre a pasta do jogo e 'pasta'; cada uma e registrada (de fora para dentro) para a
        // desinstalacao apagar, se ficarem vazias.
        static void CriarPastas(Sessao s, string raiz, string pasta)
        {
            if (Directory.Exists(pasta)) return;
            string pai = Path.GetDirectoryName(pasta.TrimEnd('\\'));
            if (pai != null && Util.Dentro(pasta, raiz)) CriarPastas(s, raiz, pai);
            s.Registrar(Acao("pasta_criada", Rel(s, pasta)));
            Directory.CreateDirectory(pasta);
        }

        // rdr2 (26/10): versao do arquivo (FileVersion numerica) ou null se nao tem recurso de versao / nao le.
        static Version VersaoArquivo(string caminho)
        {
            try
            {
                FileVersionInfo vi = FileVersionInfo.GetVersionInfo(caminho);
                if (vi.FileMajorPart == 0 && vi.FileMinorPart == 0 && vi.FileBuildPart == 0 && vi.FilePrivatePart == 0) return null;
                return new Version(vi.FileMajorPart, vi.FileMinorPart, vi.FileBuildPart, vi.FilePrivatePart);
            }
            catch (Exception) { return null; }
        }

        // Extra amd_fidelityfx_*.dll que ja existe no jogo com versao MAIOR que a do pacote: devolve a versao dele (texto) para manter;
        // null = instalar (nao existe, nao e DLL da FidelityFX API, ou alguma das versoes e desconhecida, ou a do pacote e igual/maior).
        internal static string ExtraFfxMaisNovo(string destino, string origem)
        {
            string nome = Path.GetFileName(destino).ToLowerInvariant();
            if (!nome.StartsWith("amd_fidelityfx_") || !nome.EndsWith(".dll") || !File.Exists(destino)) return null;
            Version noJogo = VersaoArquivo(destino), doPacote = VersaoArquivo(origem);
            if (noJogo == null || doPacote == null || noJogo <= doPacote) return null;
            return noJogo.ToString();
        }

        static void CopiarComBackup(Sessao s, string origem, string destino, string shaNovo)
        {
            string rel = Rel(s, destino);
            if (File.Exists(destino))
            {
                string atual = Util.Sha256(destino);
                if (atual == shaNovo) { s.Registrar(Acao("igual", rel)); s.Linha(T.F("arq_igual", rel)); return; }
                string nomeBk = GuardarBackup(s, destino, "arquivo", rel, atual);
                Dictionary<string, object> a = Acao("trocado", rel);
                a["backup"] = nomeBk; a["anterior"] = atual; a["novo"] = shaNovo;
                s.Registrar(a);
                Util.TirarSomenteLeitura(destino);   // review P2 (26/10): arquivo do jogo somente leitura (o backup ja o guardou)
                File.Copy(origem, destino, true);
                s.Linha(T.F("arq_trocado", rel, Util.Curto(atual), Util.Curto(shaNovo)));
            }
            else
            {
                Dictionary<string, object> a = Acao("acrescentado", rel);
                a["novo"] = shaNovo;
                s.Registrar(a);
                File.Copy(origem, destino);
                s.Linha(T.F("arq_novo", rel));
            }
            // review P2 (26/10): o File.Copy leva o somente leitura do arquivo do pacote para o jogo; sem tirar, a desinstalacao
            // (e a proxima atualizacao) tropecam nele
            Util.TirarSomenteLeitura(destino);
            if (Util.Sha256(destino) != shaNovo) throw new IOException(T.F("erro_copia", rel));
        }

        // 1.1.0: arquivos que o MOD cria ao jogar (o INI da proxy, completado pelo helper publico, e o amdlss5.dat das
        // escolhas do menu). Nada e escrito agora: se o arquivo nao existe, a desinstalacao o apaga; se existe (do
        // jogador ou de uma instalacao anterior), uma copia vai para o backup e volta ao desinstalar.
        static void GuardarArquivoDoMod(Sessao s, string destino)
        {
            string rel = Rel(s, destino);
            if (!File.Exists(destino))
            {
                s.Registrar(Acao("criado_pelo_mod", rel));
                s.Gravar();
                s.Linha(T.F("mod_cria", rel));
                return;
            }
            string atual = Util.Sha256(destino);
            string nomeBk = GuardarBackup(s, destino, "mod", rel, atual);
            Dictionary<string, object> a = Acao("preservado", rel);
            a["backup"] = nomeBk; a["anterior"] = atual;
            s.Registrar(a);
            s.Gravar();
            s.Linha(T.F("mod_guardado", rel));
        }

        // INI do pacote. Tres casos:
        //  - nao existe: copia (ini_novo) com o idioma do instalador;
        //  - existe e foi o mod que o criou (doMod: 1a acao numa sessao anterior = ini_novo/criado_pelo_mod): P1-15 (19/10),
        //    vira o do pacote (valores novos da versao chegam) e so as chaves do JOGADOR ficam (Ini.ChaveDoUsuario: idioma,
        //    preferencias do menu, limite de FPS, filtros, F10); o antigo vai para o backup (ini_trocado);
        //  - existe e e do jogador (ja estava antes do mod): so recebe as chaves que faltam (ini_mesclado).
        static void InstalarIni(Sessao s, string iniOrigem, string destino, string idioma, bool doMod)
        {
            string rel = Rel(s, destino);
            if (File.Exists(destino))
            {
                string anterior = Util.Sha256(destino);
                string nomeBk = GuardarBackup(s, destino, "ini", rel, anterior);
                Dictionary<string, object> a = Acao(doMod ? "ini_trocado" : "ini_mesclado", rel);
                a["backup"] = nomeBk; a["anterior"] = anterior;
                s.Registrar(a);
                if (doMod)
                {
                    List<string> mantidas = Ini.TrocarMantendoUsuario(destino, iniOrigem, idioma);
                    a["chaves"] = mantidas.ToArray();
                    s.Gravar();
                    s.Linha(T.F("ini_trocado", rel, mantidas.Count));
                }
                else
                {
                    List<string> novas = Ini.MesclarFaltantes(destino, iniOrigem);
                    a["chaves"] = novas.ToArray();
                    s.Gravar();
                    s.Linha(T.F("ini_mesclado", rel, novas.Count));
                }
            }
            else
            {
                s.Registrar(Acao("ini_novo", rel));
                File.Copy(iniOrigem, destino);
                Util.TirarSomenteLeitura(destino);
                if (!string.IsNullOrEmpty(idioma)) Ini.Definir(destino, Ini.Secao, "MenuLanguage", idioma);
                s.Linha(T.F("ini_novo", rel));
            }
        }

        static void InstalarReframework(Sessao s, string origem, string destino)
        {
            string rel = Rel(s, destino);
            string sha = Util.Sha256(origem);
            if (File.Exists(destino))
            {
                // dinput8.dll ja existe: REFramework do usuario (qualquer versao) ou outro mod. Fica o dele.
                s.Registrar(Acao("igual", rel));
                s.Linha(Util.Sha256(destino) == sha ? T.S("ref_ja") : T.S("ref_mantido"));
                return;
            }
            Dictionary<string, object> a = Acao("acrescentado", rel);
            a["novo"] = sha;
            s.Registrar(a);
            Util.CopiarConferindo(origem, destino, sha, T.F("erro_copia", rel));
            s.Linha(T.F("ref_instalado", rel));
        }

        // Desfaz uma lista de acoes (da ultima para a primeira). true = tudo voltou exatamente.
        // review P2 (26/10): cada acao no seu try -- um arquivo travado/sem permissao nao derruba o resto desta sessao nem as
        // sessoes mais velhas; a falha vai para o log (des_falhou) e o resultado fica "parcial" (o acoes.json fica: uma nova
        // desinstalacao termina o que faltou; as acoes ja desfeitas se repetem sem efeito). O somente leitura do arquivo do
        // jogo sai antes de apagar/sobrescrever.
        static bool Desfazer(List<Dictionary<string, object>> acoes, string pastaMod, string pastaSessao, IRelato r)
        {
            bool limpo = true;
            for (int k = acoes.Count - 1; k >= 0; k--)
            {
                Dictionary<string, object> a = acoes[k];
                string rel = Convert.ToString(a["caminho"]);
                try
                {
                    string tipo = Convert.ToString(a["acao"]);
                    string caminho = Path.GetFullPath(Path.Combine(pastaMod, rel));
                    if (!Util.Dentro(caminho, pastaMod)) { r.Linha(T.F("des_fora", caminho)); limpo = false; continue; }
                    object o;
                    switch (tipo)
                    {
                        case "trocado":
                            if (!RestaurarConferindo(a, caminho, rel, pastaSessao, true, r)) limpo = false;
                            break;
                        case "acrescentado":
                            {
                                if (!File.Exists(caminho)) break;
                                string novo = a.TryGetValue("novo", out o) ? Convert.ToString(o) : null;
                                if (novo != null && Util.Sha256(caminho) != novo) { r.Linha(T.F("des_mantido_usuario", rel)); break; }
                                Util.TirarSomenteLeitura(caminho);
                                File.Delete(caminho);
                                r.Linha(T.F("des_removido", rel));
                                break;
                            }
                        case "ini_novo":
                        case "criado_pelo_mod":
                            if (File.Exists(caminho)) { Util.TirarSomenteLeitura(caminho); File.Delete(caminho); r.Linha(T.F("des_removido", rel)); }
                            break;
                        case "pasta_criada":   // so se ficou vazia: nunca apaga o que o jogador pos la
                            if (Directory.Exists(caminho) && Directory.GetFileSystemEntries(caminho).Length == 0) { Directory.Delete(caminho); r.Linha(T.F("des_removido", rel)); }
                            break;
                        case "preservado":
                            if (!RestaurarConferindo(a, caminho, rel, pastaSessao, false, r)) limpo = false;
                            break;
                        case "ini_mesclado":
                        case "ini_trocado":
                            {
                                string bk = BackupDe(a, pastaSessao, r);
                                if (bk == null) { limpo = false; break; }
                                if (!File.Exists(bk)) { r.Linha(T.F("des_sem_backup", rel)); limpo = false; break; }
                                if (File.Exists(caminho)) Util.TirarSomenteLeitura(caminho);
                                File.Copy(bk, caminho, true);
                                r.Linha(T.F("des_restaurado", rel));
                                break;
                            }
                        default: break;
                    }
                }
                catch (Exception e) { r.Linha(T.F("des_falhou", rel, e.Message)); limpo = false; }
            }
            return limpo;
        }

        // rev-dup (review P2-8): "trocado" e "preservado" desfazem igual (antes duas copias): o arquivo ja e o de antes = nada;
        // senao o backup volta e e conferido pelo SHA-256 'anterior'. avisaMudou (so "trocado"): diz se o arquivo nao era mais
        // o que o mod pos. false = nao voltou exatamente.
        static bool RestaurarConferindo(Dictionary<string, object> a, string caminho, string rel, string pastaSessao, bool avisaMudou, IRelato r)
        {
            string bk = BackupDe(a, pastaSessao, r);
            if (bk == null) return false;
            string anterior = Convert.ToString(a["anterior"]);
            if (File.Exists(caminho) && Util.Sha256(caminho) == anterior) return true;
            if (!File.Exists(bk)) { r.Linha(T.F("des_sem_backup", rel)); return false; }
            object o;
            if (avisaMudou && File.Exists(caminho) && a.TryGetValue("novo", out o) && Util.Sha256(caminho) != Convert.ToString(o))
                r.Linha(T.F("des_mudou_restaura", rel));
            File.Copy(bk, caminho, true);
            if (Util.Sha256(caminho) != anterior) { r.Linha(T.F("des_divergiu", rel)); return false; }
            r.Linha(T.F("des_restaurado", rel));
            return true;
        }

        // Backup citado numa acao, so dentro da pasta da sessao (um acoes.json adulterado nao copia arquivo de fora).
        static string BackupDe(Dictionary<string, object> a, string pastaSessao, IRelato r)
        {
            object nome;
            a.TryGetValue("backup", out nome);
            string c = Util.CaminhoBackup(pastaSessao, nome);
            if (c == null) r.Linha(T.F("des_fora", Convert.ToString(nome)));
            return c;
        }

        // Desinstala tudo o que o instalador fez nesta pasta (todas as sessoes, da mais nova para a
        // mais velha). Se tudo voltou exatamente, apaga AMDLSS5_backup. O log fica em %TEMP%.
        public static string Desinstalar(Jogo j, IRelato fora)
        {
            if (j.PastaMod == null) throw new ErroInstalador(CodigoErro.Bloqueado, string.Join(Environment.NewLine, j.Bloqueios.ToArray()));
            string bkRaiz = Path.Combine(j.PastaMod, Util.PastaBackup);
            if (!Sessoes.Resta(j.PastaMod))
                throw new ErroInstalador(CodigoErro.Faltando, T.F("erro_sem_backup", j.PastaMod));
            GuardaJogoAberto(j);
            Util.TestarEscrita(j.PastaMod);
            string pastaLog = Util.TempInstalador();
            string arqLog = Path.Combine(pastaLog, "desinstalacao-" + DateTime.Now.ToString("yyyyMMdd-HHmmss") + ".log");
            Sessao log = new Sessao(j.PastaMod, pastaLog, fora, Path.GetFileName(arqLog));
            try
            {
                // pastas sem acoes.json nao desfazem nada (sobras de uma limpeza que falhou): ficam fora e nao tornam a
                // desinstalacao "parcial"
                List<string> sessoes = Sessoes.Validas(bkRaiz, false);
                sessoes.Reverse();
                bool tudo = true;
                int n = 0;
                foreach (string sessao in sessoes)
                {
                    log.Linha(T.F("des_sessao", Path.GetFileName(sessao)));
                    List<Dictionary<string, object>> acoes;
                    try { acoes = Sessoes.Ler(sessao); }
                    catch (Exception e) { log.Linha(T.F("des_sem_acoes", sessao) + " (" + e.Message + ")"); tudo = false; continue; }
                    if (!Desfazer(acoes, j.PastaMod, sessao, log)) tudo = false;
                    fora.Progresso(++n * 100 / sessoes.Count);
                }
                if (tudo)
                {
                    log.Fechar();
                    // primeiro os livros de desfazer (sem eles nada mais e aplicado de novo); depois a pasta toda
                    foreach (string sessao in sessoes) Util.ApagarAcoes(sessao);
                    try { Util.ApagarPasta(bkRaiz); }
                    catch (Exception e) { fora.Linha(T.F("des_sobra", bkRaiz, e.Message)); }
                    fora.Linha(T.F("des_ok", arqLog));
                }
                else fora.Linha(T.F("des_parcial", bkRaiz, arqLog));
                return arqLog;
            }
            finally { log.Fechar(); }
        }
    }

    // INI (o helper usa Get/WritePrivateProfileString). O arquivo e lido na codificacao em que esta (UTF-16 LE com BOM,
    // UTF-8 com BOM ou ANSI = latin1 byte a byte) e gravado de volta nela (tmp + troca atomica). Chaves por SECAO: uma chave
    // de [DlssNrOnAmd] nao conta como presente se so existir em outra secao, e a chave nova entra na sua secao (antes
    // entrava no fim do arquivo, dentro da ultima secao, qualquer que fosse).
    public static class Ini
    {
        public const string Secao = "DlssNrOnAmd";
        static readonly Regex Chave = new Regex(@"^\s*([A-Za-z0-9_]+)\s*=");
        static readonly Regex Cabecalho = new Regex(@"^\s*\[([^\]\r\n]*)\]");

        // P1-15 (19/10): chaves do JOGADOR, que uma atualizacao nunca troca pelo valor do pacote: idioma e preferencias do
        // menu (ajuda, bloqueio de entrada, painel compacto de FPS/rede), limite de FPS, liga/desliga do F10 e os filtros de
        // imagem (NeuralFiltro*). O resto e regulagem da versao e vem do pacote.
        // 26/10 (usuario: "so instalacoes de updates vao manter os valores do usuario"): tambem as outras linhas visiveis do menu --
        // estrutura da cena e da pele, intensidade do tom, estilo neural, passos neurais e Exposicao/Contraste/Saturacao da rede.
        static readonly string[] ChavesDoUsuario = { "Enabled", "MenuLanguage", "MenuShowHelp", "MenuInputBlock", "NeuralMenuCompacto", "NeuralFrameCap",
            "LocalStructure", "SkinStructure", "LocalTone", "NeuralStyle", "NeuralPassConfig", "NeuralPasses",
            "NeuralGradeExposure", "NeuralGradeContrast", "NeuralGradeSaturation" };
        static readonly string[] PrefixosDoUsuario = { "NeuralFiltro" };

        public static bool ChaveDoUsuario(string chave)
        {
            foreach (string c in ChavesDoUsuario) if (string.Equals(c, chave, StringComparison.OrdinalIgnoreCase)) return true;
            foreach (string p in PrefixosDoUsuario) if (chave.StartsWith(p, StringComparison.OrdinalIgnoreCase)) return true;
            return false;
        }

        sealed class Arquivo
        {
            public Encoding Cod;
            public byte[] Bom = new byte[0];
            public string Texto;
        }

        static Arquivo Ler(string caminho)
        {
            byte[] b = File.ReadAllBytes(caminho);
            Arquivo a = new Arquivo();
            if (b.Length >= 2 && b[0] == 0xFF && b[1] == 0xFE) { a.Cod = new UnicodeEncoding(false, false); a.Bom = new byte[] { 0xFF, 0xFE }; }
            else if (b.Length >= 3 && b[0] == 0xEF && b[1] == 0xBB && b[2] == 0xBF) { a.Cod = new UTF8Encoding(false); a.Bom = new byte[] { 0xEF, 0xBB, 0xBF }; }
            else a.Cod = Util.Latin1;
            a.Texto = a.Cod.GetString(b, a.Bom.Length, b.Length - a.Bom.Length);
            return a;
        }

        static void Gravar(string caminho, Arquivo a)
        {
            byte[] corpo = a.Cod.GetBytes(a.Texto);
            byte[] tudo = new byte[a.Bom.Length + corpo.Length];
            Buffer.BlockCopy(a.Bom, 0, tudo, 0, a.Bom.Length);
            Buffer.BlockCopy(corpo, 0, tudo, a.Bom.Length, corpo.Length);
            string tmp = caminho + ".amdlss5.tmp";
            File.WriteAllBytes(tmp, tudo);
            Util.TrocarArquivo(tmp, caminho);
        }

        // Uma linha do texto: [Inicio, FimConteudo) e o conteudo sem a quebra; FimLinha inclui a quebra.
        sealed class Linha
        {
            public int Inicio, FimConteudo, FimLinha;
            public string Secao, Chave, Valor;
            public bool Cabecalho;
        }

        static List<Linha> Linhas(string texto)
        {
            List<Linha> r = new List<Linha>();
            string secao = "";
            int pos = 0;
            while (pos < texto.Length)
            {
                int nl = texto.IndexOf('\n', pos);
                int fimLinha = nl < 0 ? texto.Length : nl + 1;
                int fim = nl < 0 ? texto.Length : nl;
                if (fim > pos && texto[fim - 1] == '\r') fim--;
                string conteudo = texto.Substring(pos, fim - pos);
                Linha l = new Linha();
                l.Inicio = pos; l.FimConteudo = fim; l.FimLinha = fimLinha;
                Match m = Cabecalho.Match(conteudo);
                if (m.Success) { secao = m.Groups[1].Value.Trim(); l.Cabecalho = true; }
                else
                {
                    Match k = Chave.Match(conteudo);
                    if (k.Success) { l.Chave = k.Groups[1].Value; l.Valor = conteudo.Substring(k.Length); }
                }
                l.Secao = secao;
                r.Add(l);
                pos = fimLinha;
            }
            return r;
        }

        static string Quebra(string texto) { return texto.Contains("\r\n") || !texto.Contains("\n") ? "\r\n" : "\n"; }

        // Onde uma linha nova da secao entra: logo depois da ultima chave (ou do cabecalho) dela. -1 = a secao nao existe.
        static int PontoDaSecao(List<Linha> linhas, string secao)
        {
            int p = -1;
            foreach (Linha l in linhas)
                if (string.Equals(l.Secao, secao, StringComparison.OrdinalIgnoreCase) && (l.Cabecalho || l.Chave != null)) p = l.FimLinha;
            if (p < 0 && secao.Length == 0) p = 0;   // chaves antes de qualquer secao: comeco do arquivo
            return p;
        }

        // Insere linhas (sem quebra) na secao; cria a secao no fim do arquivo se ela nao existir.
        static string Inserir(string texto, string secao, List<string> novas)
        {
            if (novas.Count == 0) return texto;
            string q = Quebra(texto);
            int p = PontoDaSecao(Linhas(texto), secao);
            if (p < 0)
            {
                string pre = texto.Length > 0 && !texto.EndsWith("\n") ? q : "";
                return texto + pre + "[" + secao + "]" + q + string.Join(q, novas.ToArray()) + q;
            }
            string antes = p > 0 && texto[p - 1] != '\n' ? q : "";   // ultima linha do arquivo sem quebra
            return texto.Substring(0, p) + antes + string.Join(q, novas.ToArray()) + q + texto.Substring(p);
        }

        static string DefinirTexto(string texto, string secao, string chave, string valor)
        {
            foreach (Linha l in Linhas(texto))
                if (l.Chave != null && string.Equals(l.Secao, secao, StringComparison.OrdinalIgnoreCase) && string.Equals(l.Chave, chave, StringComparison.OrdinalIgnoreCase))
                {
                    int ini = l.FimConteudo - l.Valor.Length;
                    return texto.Substring(0, ini) + valor + texto.Substring(l.FimConteudo);
                }
            return Inserir(texto, secao, new List<string> { chave + "=" + valor });
        }

        // Acrescenta ao alvo as chaves do recomendado que faltam NA MESMA SECAO (as linhas como estao no recomendado).
        // Devolve os nomes acrescentados. Nada mais muda no alvo (codificacao, quebras, comentarios, valores).
        public static List<string> MesclarFaltantes(string alvo, string recomendado)
        {
            Arquivo a = Ler(alvo);
            HashSet<string> existentes = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (Linha l in Linhas(a.Texto)) if (l.Chave != null) existentes.Add(l.Secao + "\n" + l.Chave);
            Arquivo rec = Ler(recomendado);
            List<string> ordemSecoes = new List<string>(), chaves = new List<string>();
            Dictionary<string, List<string>> novas = new Dictionary<string, List<string>>(StringComparer.OrdinalIgnoreCase);
            foreach (Linha l in Linhas(rec.Texto))
            {
                if (l.Chave == null || !existentes.Add(l.Secao + "\n" + l.Chave)) continue;
                List<string> lista;
                if (!novas.TryGetValue(l.Secao, out lista)) { lista = new List<string>(); novas[l.Secao] = lista; ordemSecoes.Add(l.Secao); }
                lista.Add(rec.Texto.Substring(l.Inicio, l.FimConteudo - l.Inicio));
                chaves.Add(l.Chave);
            }
            if (chaves.Count == 0) return chaves;
            string texto = a.Texto;
            foreach (string sec in ordemSecoes) texto = Inserir(texto, sec, novas[sec]);
            a.Texto = texto;
            Gravar(alvo, a);
            return chaves;
        }

        // P1-15: o alvo (INI criado pelo mod numa versao anterior) vira o do pacote; as chaves do jogador (ChaveDoUsuario)
        // voltam com o valor que tinham, na mesma secao. Sem MenuLanguage antigo, vale o idioma do instalador (se houver).
        // Devolve as chaves mantidas.
        public static List<string> TrocarMantendoUsuario(string alvo, string pacote, string idioma)
        {
            Arquivo velho = Ler(alvo);
            Arquivo novo = Ler(pacote);
            List<string> mantidas = new List<string>();
            HashSet<string> vistas = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            string texto = novo.Texto;
            bool temIdioma = false;
            foreach (Linha l in Linhas(velho.Texto))
            {
                if (l.Chave == null || !ChaveDoUsuario(l.Chave) || !vistas.Add(l.Secao + "\n" + l.Chave)) continue;   // a 1a ocorrencia vale (como no Windows)
                texto = DefinirTexto(texto, l.Secao, l.Chave, l.Valor);
                mantidas.Add(l.Chave);
                if (string.Equals(l.Chave, "MenuLanguage", StringComparison.OrdinalIgnoreCase) && string.Equals(l.Secao, Secao, StringComparison.OrdinalIgnoreCase)) temIdioma = true;
            }
            if (!temIdioma && !string.IsNullOrEmpty(idioma)) texto = DefinirTexto(texto, Secao, "MenuLanguage", idioma);
            novo.Texto = texto;
            Gravar(alvo, novo);
            return mantidas;
        }

        public static void Definir(string arquivo, string secao, string chave, string valor)
        {
            Arquivo a = Ler(arquivo);
            a.Texto = DefinirTexto(a.Texto, secao, chave, valor);
            Gravar(arquivo, a);
        }
    }
}
