// Ferramenta de desenvolvimento chamada por instalador\Montar-Pacote.ps1 (que a compila com o csc do
// .NET Framework e passa os caminhos ja resolvidos). Monta a pasta de distribuicao:
//   <dest>\payload\mod\*            arquivos do mod conferidos pelo manifesto do pacote (nome -> SHA-256)
//   <dest>\payload\pacote.json      manifesto lido pelo instalador
//   <dest>\payload\reframework\     dinput8.dll do REFramework (opcional) + origem/licenca
//   <dest>\payload\ini\<Exe>.ini    INIs por jogo (opcional)
//   <dest>\payload\porjogo\<exe>\<relativo>   extras por jogo (opcional; "porjogo" do pacote.json com SHA-256)
//   <dest>\payload\testados.json    jogos testados (de <modelos>\testados.json; "testados" do pacote.json = SHA-256)
//   <dest>\THIRD_PARTY_NOTICES.txt  avisos de licenca de terceiros (de <modelos>\THIRD_PARTY_NOTICES.txt; ao, 26/10)
// Nunca apaga nada; recusa se <dest> ja existe.
//
// Por que C# e nao PowerShell: o antivirus desta maquina (Bitdefender) bloqueava a versao em
// PowerShell deste script (combinacao de copiar DLLs + texto + zip), deixando o .ps1 inacessivel.
using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;

static class MontarPacote
{
    static string Sha(string p)
    {
        using (SHA256 h = SHA256.Create())
        using (FileStream fs = File.OpenRead(p))
        {
            StringBuilder sb = new StringBuilder();
            foreach (byte b in h.ComputeHash(fs)) sb.Append(b.ToString("X2"));
            return sb.ToString();
        }
    }

    static bool Contem(byte[] dados, byte[] alvo)
    {
        for (int i = Array.IndexOf(dados, alvo[0]); i >= 0 && i <= dados.Length - alvo.Length; i = Array.IndexOf(dados, alvo[0], i + 1))
        {
            int k = 1;
            while (k < alvo.Length && dados[i + k] == alvo[k]) k++;
            if (k == alvo.Length) return true;
        }
        return false;
    }

    // Chaves de diagnostico que o pacote publico leva sempre em 0 (gravacao F9, captura, despejos, traces, logs de pesquisa).
    // 19/10 (review P1-9): + NeuralLogPicoRede, NeuralRostoJogadorDiagHist, NeuralStyleDiagnostic; o padrao.ini tinha
    // NeuralFgConfigLog=1 (forcado a 0 aqui). Vale para o INI padrao E para os INIs por jogo (-IniJogo).
    // 21/10 (revisao final da biblioteca): + NeuralGravarAviso e NeuralRostoJogadorLogJobs (o re9.ini os trazia em 1) e
    // os outros logs/traces de pesquisa do helper.
    // 26/10 (review do pacote 1.6.0): + NeuralFiltrosComFg (1 = filtros/AO/IL rodando junto com a geracao de quadros do mod;
    // so pesquisa: no publico os filtros ficam sempre suspensos com o FG ligado, runtime\filtros.h).
    static readonly string[] Desligar = { "NeuralGravarF9", "NeuralGravarF9Reserva", "NeuralTelaCaptura", "NeuralFgConfigLog", "NeuralDumpFinalMovimento",
        "NeuralDumpFrames", "NeuralDumpFillStages", "NeuralDumpDepth", "NeuralKernelTrace", "NeuralContractTrace",
        "NeuralLogPicoRede", "NeuralRostoJogadorDiagHist", "NeuralStyleDiagnostic",
        "NeuralGravarAviso", "NeuralRostoJogadorLogJobs", "NeuralAgeLogFrames", "NeuralHistMovLog", "NeuralHostCopyLog", "NeuralHostGapLog",
        "NeuralCenaTrace", "NeuralConv2LaunchTrace", "NeuralEsperaDiag", "NeuralDumpFinalSeguro", "NeuralCapturaCarimbo",
        "NeuralFiltrosComFg", "NeuralTempoGpu" };   // 27/10: tempo de GPU por passe (o SOTTR.ini de pesquisa trazia 1; no -SemLog nao mede nada)

    // Trava da montagem: depois de desligar a lista acima, nenhuma chave com nome de log/gravacao/despejo/trace/diagnostico/
    // captura/sonda pode ficar diferente de 0, exceto estas (tamanhos, contagens e parametros que nao gravam nada sozinhos).
    static readonly Regex NomeDiagnostico = new Regex("Log|Grav|Dump|Trace|Diag|Captura|Sonda");
    static readonly string[] Permitidas = { "NeuralLogTetoMB", "NeuralGravarF9Quadros", "NeuralDumpGain", "NeuralDumpAfterFrames", "NeuralDumpEveryFrames",
        "NeuralKernelTraceJobs", "NeuralContractTraceBytes", "NeuralSemDiagMov",
        // pacote-160 (26/10): nao grava nada -- o recorte do rosto usa a cabeca do job (correcao do rosto piscando no SotTR, sottr-captura)
        "NeuralRostoJogadorCapturaJob" };

    // review P1-13 (26/10): binarios do pacote publico SEM nome de arquivo de log/texto/despejo embutido -- a regra de
    // analysis\v040-port\Verificar-SemLog.ps1 (mesmo padrao, ASCII e UTF-16LE nas duas paridades de byte), aplicada a TODO
    // .dll/.asi de payload\mod e payload\porjogo (helper, proxy, libxess, ffx_fsr2_api_x64.dll, plugin do REFramework...).
    // Um nome desses quer dizer que um escritor de arquivo escapou das guardas NR_LOG_ATIVO / NR_SEM_GRAVACAO. Pulado so com
    // -ManterDiagnostico. Padroes recusados: *.log *.txt *.csv *.raw *.bmp *.png *.json e nr_*.ini.
    static readonly Regex NomeArquivoLog = new Regex(@"(?i)(?<![A-Za-z0-9_%\-\.])([A-Za-z0-9_%\-\.]{1,96}\.(?:log|txt|csv|raw|bmp|png|json)|nr_[A-Za-z0-9_%\-\.]{0,96}\.ini)(?![A-Za-z0-9_])");
    // Nomes citados sem que o binario os grave (acrescentar so com o motivo ao lado).
    static readonly string[] NomesPermitidos = {
        "RenoDX_LICENSE.txt",   // comentario de licenca dentro do HLSL embutido no helper (compose.hlsl); nao e arquivo
        "nr_xess_fsr.ini",      // INI que a libxess (e a ajuda do menu do helper) so LE; vai no pacote como extra do SotTR
        "nr_fsr2_ffx.ini" };    // INI que a ffx_fsr2_api_x64.dll nossa (RDR2) so LE; vai no pacote ao lado dela
    // Nomes permitidos so num arquivo do payload (rotulo "mod/<nome>" ou "porjogo/<exe>/<relativo>").
    static readonly Dictionary<string, string[]> NomesPermitidosPorArquivo = new Dictionary<string, string[]>(StringComparer.OrdinalIgnoreCase) {
        // o proxy pinado (version.dll, binario do proxy original + patches, nao e recompilado aqui) grava o dlssnr_on_amd.log
        // desde a 1.3.0 (estado-atual 18/10: "so sobra o dlssnr_on_amd.log do proxy"); outro nome novo nele derruba a montagem
        { "mod/version.dll", new[] { "_dlssnr_on_amd.log" } } };
    // Binarios de TERCEIROS, sem modificacao, que citam nomes desses legitimamente (os nossos nunca entram aqui): runtime HIP
    // (fpslog/timelog/*.csv, hip_clr_trace.json...), compilador comgr (LLVM: *.txt/*.json) e a DLL de rede do fabricante
    // (nvngx_%ls...log). Os outros de terceiros do pacote (amd_fidelityfx_*, msvcp/vcruntime) estao limpos e sao varridos.
    static readonly string[] TerceirosSemVarredura = { "mod/amdhip64_7.dll", "mod/amd_comgr.dll", "mod/nvngx_dlssnr.dll" };

    static bool NaLista(string[] lista, string v)
    {
        if (lista != null) foreach (string k in lista) if (string.Equals(k, v, StringComparison.OrdinalIgnoreCase)) return true;
        return false;
    }

    // Nomes de log/texto/despejo dentro do binario ("nome [ascii,utf16]"), fora das listas permitidas.
    static List<string> NomesDeLog(string arquivo, string rotulo)
    {
        byte[] b = File.ReadAllBytes(arquivo);
        string[] doArquivo;
        NomesPermitidosPorArquivo.TryGetValue(rotulo, out doArquivo);
        List<KeyValuePair<string, string>> textos = new List<KeyValuePair<string, string>>();
        textos.Add(new KeyValuePair<string, string>("ascii", Encoding.GetEncoding(28591).GetString(b)));
        textos.Add(new KeyValuePair<string, string>("utf16", Encoding.Unicode.GetString(b, 0, b.Length - (b.Length % 2))));
        if (b.Length > 2) textos.Add(new KeyValuePair<string, string>("utf16+1", Encoding.Unicode.GetString(b, 1, (b.Length - 1) - ((b.Length - 1) % 2))));
        SortedDictionary<string, string> achados = new SortedDictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (KeyValuePair<string, string> t in textos)
            foreach (Match m in NomeArquivoLog.Matches(t.Value))
            {
                string v = m.Groups[1].Value.TrimStart('.');
                if (v.Length < 5 || NaLista(NomesPermitidos, v) || NaLista(doArquivo, v)) continue;
                string ja;
                if (!achados.TryGetValue(v, out ja)) achados[v] = t.Key;
                else if (Array.IndexOf(ja.Split(','), t.Key) < 0) achados[v] = ja + "," + t.Key;
            }
        List<string> r = new List<string>();
        foreach (KeyValuePair<string, string> kv in achados) r.Add(kv.Key + " [" + kv.Value + "]");
        return r;
    }

    // Varre um binario copiado para o payload (so .dll/.asi); achados vao para 'falhas' como "rotulo: nome [cod], ...".
    static void VarrerSemLog(string arquivo, string rotulo, List<string> falhas, List<string> varridos, List<string> pulados)
    {
        string ext = Path.GetExtension(arquivo).ToLowerInvariant();
        if (ext != ".dll" && ext != ".asi") return;
        if (NaLista(TerceirosSemVarredura, rotulo)) { pulados.Add(rotulo); return; }
        varridos.Add(rotulo);
        List<string> nomes = NomesDeLog(arquivo, rotulo);
        if (nomes.Count > 0) falhas.Add(rotulo + ": " + string.Join(", ", nomes.ToArray()));
    }

    static bool Zero(string v)
    {
        v = v.Trim();
        double d;
        return v == "0" || (double.TryParse(v, System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out d) && d == 0);
    }

    // INI do pacote publico, linha a linha (os INIs de pesquisa misturam LF, CRLF e CR; cada linha guarda o seu fim):
    //  - comentario (';') sai: sao notas internas de pesquisa;
    //  - NeuralRostoJogadorYunet* sai (detector descontinuado, chave inerte);
    //  - chave da lista Desligar com valor diferente de 0 vira 0 (qualquer secao);
    //  - trava: chave de diagnostico fora de Permitidas que ainda nao e 0 = a montagem FALHA (lista Desligar incompleta).
    // As mudancas vao para a saida (rotulo = qual INI).
    static string PrepararIniPublico(string texto, string rotulo, List<string> mudancas)
    {
        StringBuilder sb = new StringBuilder(texto.Length);
        List<string> sobras = new List<string>();
        int comentarios = 0;
        foreach (Match m in Regex.Matches(texto, @"([^\r\n]*)(\r\n|\r|\n|$)"))
        {
            if (m.Length == 0) continue;   // o casamento vazio do fim
            string linha = m.Groups[1].Value, fim = m.Groups[2].Value;
            string t = linha.TrimStart();
            if (t.StartsWith(";")) { comentarios++; continue; }
            int eq = t.IndexOf('=');
            string chave = eq > 0 ? t.Substring(0, eq).Trim() : "";
            if (chave.StartsWith("NeuralRostoJogadorYunet", StringComparison.OrdinalIgnoreCase)) { mudancas.Add(rotulo + ": " + chave + " removida"); continue; }
            if (chave.Length > 0)
            {
                string valor = t.Substring(eq + 1);
                bool desligar = false;
                foreach (string k in Desligar) if (string.Equals(k, chave, StringComparison.OrdinalIgnoreCase)) desligar = true;
                if (desligar && valor.Trim() != "0")
                {
                    mudancas.Add(rotulo + ": " + chave + " " + valor.Trim() + " -> 0");
                    linha = linha.Substring(0, linha.Length - t.Length) + t.Substring(0, eq + 1) + "0";
                    valor = "0";
                }
                bool permitida = false;
                foreach (string k in Permitidas) if (string.Equals(k, chave, StringComparison.OrdinalIgnoreCase)) permitida = true;
                if (!permitida && NomeDiagnostico.IsMatch(chave) && !Zero(valor)) sobras.Add(chave + "=" + valor.Trim());
            }
            sb.Append(linha).Append(fim);
        }
        if (comentarios > 0) mudancas.Add(rotulo + ": " + comentarios + " linha(s) de comentario removida(s)");
        if (sobras.Count > 0)
            throw new Exception(rotulo + ": chave(s) de log/gravacao/diagnostico ligada(s) no INI publico: " + string.Join(", ", sobras.ToArray())
                + " (ponha na lista Desligar ou em Permitidas de ferramentas/MontarPacote.cs, ou use -ManterDiagnostico)");
        return sb.ToString();
    }

    // args: pacote manifesto helper ini refDir(-) dest versao manterDiag(0/1) modelos [Exe=ini ...] [@exe:relativo=origem ...]
    static int Main(string[] a)
    {
        try
        {
            string pacote = a[0], manifestoArq = a[1], helper = a[2], ini = a[3], refDir = a[4], dest = a[5], versao = a[6];
            bool manterDiag = a[7] == "1";
            if (Directory.Exists(dest)) throw new Exception(dest + " ja existe: apague-a ou use outra -Versao");
            JavaScriptSerializer js = new JavaScriptSerializer();
            Dictionary<string, object> manifesto = (Dictionary<string, object>)js.DeserializeObject(File.ReadAllText(manifestoArq));
            string payload = Path.Combine(dest, "payload");
            string mod = Path.Combine(payload, "mod");
            Directory.CreateDirectory(mod);

            SortedDictionary<string, string> arquivos = new SortedDictionary<string, string>(StringComparer.Ordinal);
            foreach (KeyValuePair<string, object> kv in manifesto)
            {
                string n = kv.Key;
                if (n == "dlssnr_on_amd.ini") continue;
                string origem = n == "nr_native_adapter.dll" ? helper : Path.Combine(pacote, n);
                if (!File.Exists(origem)) throw new Exception("falta " + origem);
                string h = Sha(origem);
                if (n != "nr_native_adapter.dll" && h != Convert.ToString(kv.Value).ToUpperInvariant())
                    throw new Exception(n + " diverge do manifesto (" + h.Substring(0, 8) + ")");
                File.Copy(origem, Path.Combine(mod, n));
                arquivos[n] = h;
            }
            // P1-13 (26/10): binarios sem log (achados juntados; a montagem falha depois dos extras, com todos de uma vez)
            List<string> falhasLog = new List<string>(), varridos = new List<string>(), pulados = new List<string>();
            if (!manterDiag)
                foreach (string n in arquivos.Keys) VarrerSemLog(Path.Combine(mod, n), "mod/" + n, falhasLog, varridos, pulados);

            // o helper guarda o SHA-256 do proxy com que foi feito; com outro proxy ele fica inerte
            string proxy = arquivos["version.dll"];
            byte[] alvo = new byte[32];
            for (int i = 0; i < 32; i++) alvo[i] = Convert.ToByte(proxy.Substring(i * 2, 2), 16);
            if (!Contem(File.ReadAllBytes(Path.Combine(mod, "nr_native_adapter.dll")), alvo))
                throw new Exception("o helper " + helper + " nao foi feito para o version.dll do pacote (" + proxy.Substring(0, 8) + "): recompile com este proxy");

            // INI: diagnostico de pesquisa desligado (gravacao F9, captura de tela, logs, despejos)
            // 1.1.0: ini "-" = pacote SEM INI (o helper publico cria o INI da proxy no primeiro jogo).
            Encoding l1 = Encoding.GetEncoding(28591);
            bool comIni = ini != "-";
            List<string> mudancas = new List<string>();
            if (comIni)
            {
                string texto = l1.GetString(File.ReadAllBytes(ini));
                if (!manterDiag) texto = PrepararIniPublico(texto, "dlssnr_on_amd.ini", mudancas);
                string iniDest = Path.Combine(mod, "dlssnr_on_amd.ini");
                File.WriteAllBytes(iniDest, l1.GetBytes(texto));
                arquivos["dlssnr_on_amd.ini"] = Sha(iniDest);
            }
            // INIs por jogo (-IniJogo Exe=ini): mesmo diagnostico desligado e hash em "inis" do pacote.json (o instalador confere)
            string modelos = a[8];
            SortedDictionary<string, string> inis = new SortedDictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            // extras por jogo (@exe:relativo/com/barras=origem): copiados para payload\porjogo\<exe>\<relativo>
            SortedDictionary<string, SortedDictionary<string, string>> porJogo = new SortedDictionary<string, SortedDictionary<string, string>>(StringComparer.Ordinal);
            for (int i = 9; i < a.Length; i++)
            {
                int eq = a[i].IndexOf('=');
                if (eq <= 0) continue;
                if (a[i][0] == '@')
                {
                    int dp = a[i].IndexOf(':');
                    if (dp < 2 || dp > eq) throw new Exception("-PorJogo: argumento invalido: " + a[i]);
                    string exeX = a[i].Substring(1, dp - 1).ToLowerInvariant();
                    if (exeX.EndsWith(".exe", StringComparison.Ordinal)) exeX = exeX.Substring(0, exeX.Length - 4);
                    string relX = a[i].Substring(dp + 1, eq - dp - 1).Replace('\\', '/');
                    if (exeX.Length == 0 || exeX.IndexOfAny(new char[] { '\\', '/', ':' }) >= 0 || exeX.Contains("..") || exeX == "_comum") throw new Exception("-PorJogo: nome de exe invalido: " + exeX);
                    if (relX.Length == 0 || relX.Contains(":") || relX.Contains("..") || relX.StartsWith("/") || relX.EndsWith("/") || relX.Contains("//")) throw new Exception("-PorJogo: caminho relativo invalido: " + relX);
                    string origemX = a[i].Substring(eq + 1);
                    if (!File.Exists(origemX)) throw new Exception("-PorJogo: falta " + origemX);
                    string destX = Path.Combine(payload, "porjogo", exeX, relX.Replace('/', '\\'));
                    if (File.Exists(destX)) throw new Exception("-PorJogo repetido: " + exeX + "/" + relX);
                    Directory.CreateDirectory(Path.GetDirectoryName(destX));
                    string rotuloX = "porjogo/" + exeX + "/" + relX;
                    // P1-13 (26/10): INI extra (nr_xess_fsr.ini, nr_fsr2_ffx.ini...) passa pelo mesmo preparo dos INIs publicos
                    // (sem comentarios internos, log/gravacao em 0); DLL/ASI extra passa pela varredura sem log
                    if (!manterDiag && relX.EndsWith(".ini", StringComparison.OrdinalIgnoreCase))
                        File.WriteAllBytes(destX, l1.GetBytes(PrepararIniPublico(l1.GetString(File.ReadAllBytes(origemX)), rotuloX, mudancas)));
                    else File.Copy(origemX, destX);
                    if (!manterDiag) VarrerSemLog(destX, rotuloX, falhasLog, varridos, pulados);
                    if (!porJogo.ContainsKey(exeX)) porJogo[exeX] = new SortedDictionary<string, string>(StringComparer.Ordinal);
                    porJogo[exeX][relX] = Sha(destX);
                    continue;
                }
                string exe = a[i].Substring(0, eq);
                if (exe.EndsWith(".exe", StringComparison.OrdinalIgnoreCase)) exe = exe.Substring(0, exe.Length - 4);
                if (exe.IndexOfAny(new char[] { '\\', '/', ':' }) >= 0 || exe.Contains("..")) throw new Exception("-IniJogo: nome de exe invalido: " + exe);
                string pastaIni = Path.Combine(payload, "ini");
                Directory.CreateDirectory(pastaIni);
                string nome = exe + ".ini";
                string destJogo = Path.Combine(pastaIni, nome);
                if (File.Exists(destJogo)) throw new Exception("-IniJogo repetido: " + nome);
                string textoJogo = l1.GetString(File.ReadAllBytes(a[i].Substring(eq + 1)));
                if (!manterDiag) textoJogo = PrepararIniPublico(textoJogo, "ini/" + nome, mudancas);
                File.WriteAllBytes(destJogo, l1.GetBytes(textoJogo));
                inis[nome] = Sha(destJogo);
            }
            // pacote-160 (27/10): extra igual (mesmo nome e SHA-256) em mais de um jogo -- a DLL de FG 4.0.1 do SotTR e do RDR2 -- vai uma
            // vez so para payload\porjogo\_comum\<SHA>\<nome>; o pacote.json continua listando o extra em cada jogo e o instalador acha a
            // copia comum pelo hash quando a pasta do jogo nao tem o arquivo. INIs ficam por jogo (pequenos e preparados por jogo).
            Dictionary<string, List<string>> iguais = new Dictionary<string, List<string>>(StringComparer.OrdinalIgnoreCase);
            foreach (KeyValuePair<string, SortedDictionary<string, string>> jg in porJogo)
                foreach (KeyValuePair<string, string> kv in jg.Value)
                {
                    if (kv.Key.EndsWith(".ini", StringComparison.OrdinalIgnoreCase)) continue;
                    string chaveIgual = kv.Value + "|" + Path.GetFileName(kv.Key.Replace('/', '\\'));
                    if (!iguais.ContainsKey(chaveIgual)) iguais[chaveIgual] = new List<string>();
                    iguais[chaveIgual].Add(Path.Combine(payload, "porjogo", jg.Key, kv.Key.Replace('/', '\\')));
                }
            foreach (KeyValuePair<string, List<string>> g in iguais)
            {
                if (g.Value.Count < 2) continue;
                string[] partes = g.Key.Split('|');
                string destComum = Path.Combine(payload, "porjogo", "_comum", partes[0], partes[1]);
                Directory.CreateDirectory(Path.GetDirectoryName(destComum));
                File.Move(g.Value[0], destComum);
                for (int k = 1; k < g.Value.Count; k++) File.Delete(g.Value[k]);
                if (Sha(destComum) != partes[0]) throw new Exception("extra comum com hash errado: " + destComum);
                Console.WriteLine("comum=" + partes[1] + " " + partes[0] + " (" + g.Value.Count + " jogos)");
            }
            if (falhasLog.Count > 0)
                throw new Exception("binario(s) com nome de arquivo de log/texto/despejo embutido no pacote publico (feito sem NR_SEM_LOG/NR_SEM_GRAVACAO?): "
                    + string.Join(" | ", falhasLog.ToArray())
                    + " (nome que o binario so le: NomesPermitidos em ferramentas/MontarPacote.cs, com o motivo; ou use -ManterDiagnostico)");

            Dictionary<string, object> doc = new Dictionary<string, object>();
            doc["produto"] = "AMDLSS5 MOD";
            doc["versao"] = versao;
            doc["montado"] = DateTime.Now.ToString("s");
            doc["arquivos"] = arquivos;
            if (inis.Count > 0) doc["inis"] = inis;
            if (porJogo.Count > 0) doc["porjogo"] = porJogo;
            string testadosOrigem = Path.Combine(modelos, "testados.json");
            if (!File.Exists(testadosOrigem)) throw new Exception("falta " + testadosOrigem);
            js.DeserializeObject(File.ReadAllText(testadosOrigem));   // JSON invalido derruba a montagem, nao o jogador
            File.Copy(testadosOrigem, Path.Combine(payload, "testados.json"));
            doc["testados"] = Sha(Path.Combine(payload, "testados.json"));
            string revisao = "";
            if (refDir != "-")
            {
                string dll = Path.Combine(refDir, "dinput8.dll");
                if (!File.Exists(dll)) throw new Exception("REFramework nao encontrado: " + dll);
                string rd = Path.Combine(payload, "reframework");
                Directory.CreateDirectory(rd);
                File.Copy(dll, Path.Combine(rd, "dinput8.dll"));
                File.Copy(Path.Combine(modelos, "REFramework-LEIA.txt"), Path.Combine(rd, "LEIA.txt"));
                foreach (string extra in new[] { "reframework_revision.txt", "origem.txt" })
                    if (File.Exists(Path.Combine(refDir, extra))) File.Copy(Path.Combine(refDir, extra), Path.Combine(rd, extra));
                string origem = Path.Combine(refDir, "origem.txt");
                if (File.Exists(origem))
                    foreach (string l in File.ReadAllLines(origem)) if (l.StartsWith("release=")) revisao = l.Substring(8);
                Dictionary<string, object> r = new Dictionary<string, object>();
                r["arquivo"] = "reframework/dinput8.dll";
                r["sha256"] = Sha(Path.Combine(rd, "dinput8.dll"));
                r["revisao"] = revisao;
                doc["reframework"] = r;
            }
            File.WriteAllText(Path.Combine(payload, "pacote.json"), js.Serialize(doc), new UTF8Encoding(false));
            File.WriteAllText(Path.Combine(dest, "LEIA-ME.txt"), File.ReadAllText(Path.Combine(modelos, "LEIA-ME.txt")).Replace("{VERSAO}", versao), new UTF8Encoding(true));
            // ao (26/10): avisos de licenca (MIT) do que o mod usa de terceiros, copiados do modelo sem mudanca; sem o modelo a montagem falha
            string avisos = Path.Combine(modelos, "THIRD_PARTY_NOTICES.txt");
            if (!File.Exists(avisos)) throw new Exception("falta " + avisos);
            File.Copy(avisos, Path.Combine(dest, "THIRD_PARTY_NOTICES.txt"));

            Console.WriteLine("arquivos=" + arquivos.Count);
            Console.WriteLine("helper=" + arquivos["nr_native_adapter.dll"]);
            Console.WriteLine("proxy=" + proxy);
            Console.WriteLine("ini=" + (comIni ? arquivos["dlssnr_on_amd.ini"] : "nenhum (o helper cria o INI da proxy)"));
            foreach (KeyValuePair<string, string> kv in inis) Console.WriteLine("ini_jogo=" + kv.Key + " " + kv.Value);
            foreach (KeyValuePair<string, SortedDictionary<string, string>> jg in porJogo)
                foreach (KeyValuePair<string, string> kv in jg.Value) Console.WriteLine("porjogo=" + jg.Key + "/" + kv.Key + " " + kv.Value);
            Console.WriteLine("testados=" + doc["testados"]);
            Console.WriteLine("ini_mudancas=" + string.Join("; ", mudancas.ToArray()));
            Console.WriteLine("semlog=" + (manterDiag ? "nao varrido (-ManterDiagnostico)" : varridos.Count + " binario(s) sem nome de log; terceiros sem varredura: " + (pulados.Count > 0 ? string.Join(", ", pulados.ToArray()) : "-")));
            Console.WriteLine("reframework=" + (doc.ContainsKey("reframework") ? ((Dictionary<string, object>)doc["reframework"])["sha256"] + " " + revisao : "-"));
            return 0;
        }
        catch (Exception e)
        {
            Console.Error.WriteLine("ERRO: " + e.Message);
            return 1;
        }
    }
}
