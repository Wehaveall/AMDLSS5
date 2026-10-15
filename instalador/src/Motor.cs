// Motor do instalador: deteccao do jogo, leitura/verificacao do pacote (payload), instalacao com
// backup e desinstalacao que restaura o backup byte a byte. Sem interface: a janela (Janela.cs) e
// o modo silencioso (Programa.cs) chamam estas classes e recebem o andamento por IRelato.
//
// Regras (mesmas do mod/Install-Game.ps1):
//  - nada fora da pasta do jogo e escrito (o download do REFramework vai para %TEMP%\AMDLSS5-Instalador);
//  - todo arquivo do jogo que seria sobrescrito vai antes para <pasta do exe>\AMDLSS5_backup\<data>\;
//  - acoes.json e regravado a cada passo (antes da escrita arriscada): uma falha no meio desfaz sozinha;
//  - o INI nunca e sobrescrito: se ja existe, so recebe as chaves que faltam (latin1, byte a byte);
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
            j.ExeImportaVersion = pe != null && pe.ImportaDll("version.dll");
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
            if (Directory.Exists(bk)) j.SessoesBackup = Directory.GetDirectories(bk).Length;
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

        public void Gravar()
        {
            string tmp = Path.Combine(Pasta, "acoes.json.tmp");
            File.WriteAllText(tmp, new JavaScriptSerializer().Serialize(Acoes), new UTF8Encoding(false));
            string fin = Path.Combine(Pasta, "acoes.json");
            if (File.Exists(fin)) File.Delete(fin);
            File.Move(tmp, fin);
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

            // espaco
            long bytes = 0;
            foreach (string c in origem.Values) bytes += new FileInfo(c).Length;
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
            string nomeSessao = DateTime.Now.ToString("yyyyMMdd-HHmmss");
            string pastaSessao = Path.Combine(bkRaiz, nomeSessao);
            for (int k = 2; Directory.Exists(pastaSessao); k++) pastaSessao = Path.Combine(bkRaiz, nomeSessao + "-" + k);
            Directory.CreateDirectory(pastaSessao);
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
                    if (iniOrigem != null) InstalarIni(s, iniOrigem, Path.Combine(alvo, Util.NomeIni), o.Idioma);
                    foreach (string n in Util.ArquivosDoMod)   // 1.3.0: com INI no pacote, os outros arquivos que o mod cria tambem
                        if (iniOrigem == null || !n.Equals(Util.NomeIni, StringComparison.OrdinalIgnoreCase)) GuardarArquivoDoMod(s, Path.Combine(alvo, n));
                    s.Progresso(30 + ++feito * 70 / total);
                }
                if (refDll != null) InstalarReframework(s, refDll, Path.Combine(j.PastaMod, "dinput8.dll"));
                s.Progresso(100);
                s.Linha(T.F("instalado_ok", pastaSessao));
                if (!j.TemStorage && j.ReEngine) s.Linha(T.S("nota_storage"));
                return pastaSessao;
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
                        Directory.Delete(pastaSessao, true);
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

        static void CopiarComBackup(Sessao s, string origem, string destino, string shaNovo)
        {
            string rel = Rel(s, destino);
            if (File.Exists(destino))
            {
                string atual = Util.Sha256(destino);
                if (atual == shaNovo) { s.Registrar(Acao("igual", rel)); s.Linha(T.F("arq_igual", rel)); return; }
                string nomeBk = "arquivo-" + Guid.NewGuid().ToString("N").Substring(0, 8) + "-" + Path.GetFileName(destino);
                string bk = Path.Combine(s.Pasta, nomeBk);
                File.Copy(destino, bk);
                if (Util.Sha256(bk) != atual) throw new IOException(T.F("erro_backup", rel));
                Dictionary<string, object> a = Acao("trocado", rel);
                a["backup"] = nomeBk; a["anterior"] = atual; a["novo"] = shaNovo;
                s.Registrar(a);
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
            string nomeBk = "mod-" + Guid.NewGuid().ToString("N").Substring(0, 8) + "-" + Path.GetFileName(destino);
            string bk = Path.Combine(s.Pasta, nomeBk);
            File.Copy(destino, bk);
            if (Util.Sha256(bk) != atual) throw new IOException(T.F("erro_backup", rel));
            Dictionary<string, object> a = Acao("preservado", rel);
            a["backup"] = nomeBk; a["anterior"] = atual;
            s.Registrar(a);
            s.Gravar();
            s.Linha(T.F("mod_guardado", rel));
        }

        static void InstalarIni(Sessao s, string iniOrigem, string destino, string idioma)
        {
            string rel = Rel(s, destino);
            if (File.Exists(destino))
            {
                string nomeBk = "ini-" + Guid.NewGuid().ToString("N").Substring(0, 8) + "-" + Path.GetFileName(destino);
                File.Copy(destino, Path.Combine(s.Pasta, nomeBk));
                Dictionary<string, object> a = Acao("ini_mesclado", rel);
                a["backup"] = nomeBk; a["anterior"] = Util.Sha256(destino);
                s.Registrar(a);
                List<string> novas = Ini.MesclarFaltantes(destino, iniOrigem);
                a["chaves"] = novas.ToArray();
                s.Gravar();
                s.Linha(T.F("ini_mesclado", rel, novas.Count));
            }
            else
            {
                s.Registrar(Acao("ini_novo", rel));
                File.Copy(iniOrigem, destino);
                if (!string.IsNullOrEmpty(idioma)) Ini.Definir(destino, "MenuLanguage", idioma);
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
            File.Copy(origem, destino);
            if (Util.Sha256(destino) != sha) throw new IOException(T.F("erro_copia", rel));
            s.Linha(T.F("ref_instalado", rel));
        }

        // Desfaz uma lista de acoes (da ultima para a primeira). true = tudo voltou exatamente.
        static bool Desfazer(List<Dictionary<string, object>> acoes, string pastaMod, string pastaSessao, IRelato r)
        {
            bool limpo = true;
            for (int k = acoes.Count - 1; k >= 0; k--)
            {
                Dictionary<string, object> a = acoes[k];
                string tipo = Convert.ToString(a["acao"]);
                string caminho = Path.GetFullPath(Path.Combine(pastaMod, Convert.ToString(a["caminho"])));
                if (!Util.Dentro(caminho, pastaMod)) { r.Linha(T.F("des_fora", caminho)); limpo = false; continue; }
                string rel = Convert.ToString(a["caminho"]);
                object o;
                switch (tipo)
                {
                    case "trocado":
                        {
                            string bk = Path.Combine(pastaSessao, Convert.ToString(a["backup"]));
                            string anterior = Convert.ToString(a["anterior"]);
                            if (File.Exists(caminho) && Util.Sha256(caminho) == anterior) break;
                            if (!File.Exists(bk)) { r.Linha(T.F("des_sem_backup", rel)); limpo = false; break; }
                            if (File.Exists(caminho) && a.TryGetValue("novo", out o) && Util.Sha256(caminho) != Convert.ToString(o))
                                r.Linha(T.F("des_mudou_restaura", rel));
                            File.Copy(bk, caminho, true);
                            if (Util.Sha256(caminho) != anterior) { r.Linha(T.F("des_divergiu", rel)); limpo = false; }
                            else r.Linha(T.F("des_restaurado", rel));
                            break;
                        }
                    case "acrescentado":
                        {
                            if (!File.Exists(caminho)) break;
                            string novo = a.TryGetValue("novo", out o) ? Convert.ToString(o) : null;
                            if (novo != null && Util.Sha256(caminho) != novo) { r.Linha(T.F("des_mantido_usuario", rel)); break; }
                            File.Delete(caminho);
                            r.Linha(T.F("des_removido", rel));
                            break;
                        }
                    case "ini_novo":
                    case "criado_pelo_mod":
                        if (File.Exists(caminho)) { File.Delete(caminho); r.Linha(T.F("des_removido", rel)); }
                        break;
                    case "preservado":
                        {
                            string bk = Path.Combine(pastaSessao, Convert.ToString(a["backup"]));
                            string anterior = Convert.ToString(a["anterior"]);
                            if (File.Exists(caminho) && Util.Sha256(caminho) == anterior) break;
                            if (!File.Exists(bk)) { r.Linha(T.F("des_sem_backup", rel)); limpo = false; break; }
                            File.Copy(bk, caminho, true);
                            if (Util.Sha256(caminho) != anterior) { r.Linha(T.F("des_divergiu", rel)); limpo = false; }
                            else r.Linha(T.F("des_restaurado", rel));
                            break;
                        }
                    case "ini_mesclado":
                        {
                            string bk = Path.Combine(pastaSessao, Convert.ToString(a["backup"]));
                            if (!File.Exists(bk)) { r.Linha(T.F("des_sem_backup", rel)); limpo = false; break; }
                            File.Copy(bk, caminho, true);
                            r.Linha(T.F("des_restaurado", rel));
                            break;
                        }
                    default: break;
                }
            }
            return limpo;
        }

        // Desinstala tudo o que o instalador fez nesta pasta (todas as sessoes, da mais nova para a
        // mais velha). Se tudo voltou exatamente, apaga AMDLSS5_backup. O log fica em %TEMP%.
        public static string Desinstalar(Jogo j, IRelato fora)
        {
            if (j.PastaMod == null) throw new ErroInstalador(CodigoErro.Bloqueado, string.Join(Environment.NewLine, j.Bloqueios.ToArray()));
            string bkRaiz = Path.Combine(j.PastaMod, Util.PastaBackup);
            if (!Directory.Exists(bkRaiz) || Directory.GetDirectories(bkRaiz).Length == 0)
                throw new ErroInstalador(CodigoErro.Faltando, T.F("erro_sem_backup", j.PastaMod));
            GuardaJogoAberto(j);
            Util.TestarEscrita(j.PastaMod);
            string pastaLog = Util.TempInstalador();
            string arqLog = Path.Combine(pastaLog, "desinstalacao-" + DateTime.Now.ToString("yyyyMMdd-HHmmss") + ".log");
            Sessao log = new Sessao(j.PastaMod, pastaLog, fora, Path.GetFileName(arqLog));
            try
            {
                List<string> sessoes = new List<string>(Directory.GetDirectories(bkRaiz));
                sessoes.Sort(StringComparer.Ordinal);
                sessoes.Reverse();
                bool tudo = true;
                int n = 0;
                foreach (string sessao in sessoes)
                {
                    string json = Path.Combine(sessao, "acoes.json");
                    log.Linha(T.F("des_sessao", Path.GetFileName(sessao)));
                    if (!File.Exists(json)) { log.Linha(T.F("des_sem_acoes", sessao)); tudo = false; continue; }
                    List<Dictionary<string, object>> acoes = new List<Dictionary<string, object>>();
                    object[] lista = new JavaScriptSerializer().DeserializeObject(File.ReadAllText(json, Encoding.UTF8)) as object[];
                    if (lista != null) foreach (object o in lista) acoes.Add((Dictionary<string, object>)o);
                    if (!Desfazer(acoes, j.PastaMod, sessao, log)) tudo = false;
                    fora.Progresso(++n * 100 / sessoes.Count);
                }
                if (tudo)
                {
                    log.Fechar();
                    Directory.Delete(bkRaiz, true);
                    fora.Linha(T.F("des_ok", arqLog));
                }
                else fora.Linha(T.F("des_parcial", bkRaiz, arqLog));
                return arqLog;
            }
            finally { log.Fechar(); }
        }
    }

    // INI: edicao byte a byte em latin1 (o helper usa Get/WritePrivateProfileString).
    public static class Ini
    {
        static readonly Regex Chave = new Regex(@"^\s*([A-Za-z0-9_]+)\s*=");

        public static List<string> MesclarFaltantes(string alvo, string recomendado)
        {
            Encoding l1 = Util.Latin1;
            byte[] bytes = File.ReadAllBytes(alvo);
            string texto = l1.GetString(bytes);
            HashSet<string> existentes = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (string linha in Regex.Split(texto, "\r\n|\n"))
            {
                Match m = Chave.Match(linha);
                if (m.Success) existentes.Add(m.Groups[1].Value);
            }
            List<string> novas = new List<string>(), chaves = new List<string>();
            foreach (string linha in Regex.Split(l1.GetString(File.ReadAllBytes(recomendado)), "\r\n|\n"))
            {
                Match m = Chave.Match(linha);
                if (m.Success && existentes.Add(m.Groups[1].Value)) { novas.Add(linha.TrimEnd('\r')); chaves.Add(m.Groups[1].Value); }
            }
            if (novas.Count == 0) return chaves;
            string sufixo = (texto.Length > 0 && !texto.EndsWith("\n") ? "\r\n" : "") + string.Join("\r\n", novas.ToArray()) + "\r\n";
            using (FileStream fs = new FileStream(alvo, FileMode.Append, FileAccess.Write))
            {
                byte[] b = l1.GetBytes(sufixo);
                fs.Write(b, 0, b.Length);
            }
            return chaves;
        }

        public static void Definir(string arquivo, string chave, string valor)
        {
            Encoding l1 = Util.Latin1;
            string texto = l1.GetString(File.ReadAllBytes(arquivo));
            Regex re = new Regex(@"(?m)^(\s*" + Regex.Escape(chave) + @"\s*=)[^\r\n]*");
            if (re.IsMatch(texto)) texto = re.Replace(texto, "${1}" + valor, 1);
            else texto += (texto.Length > 0 && !texto.EndsWith("\n") ? "\r\n" : "") + chave + "=" + valor + "\r\n";
            File.WriteAllBytes(arquivo, l1.GetBytes(texto));
        }
    }
}
