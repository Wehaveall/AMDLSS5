// Ferramenta de desenvolvimento chamada por instalador\Montar-Pacote.ps1 (que a compila com o csc do
// .NET Framework e passa os caminhos ja resolvidos). Monta a pasta de distribuicao:
//   <dest>\payload\mod\*            arquivos do mod conferidos pelo manifesto do pacote (nome -> SHA-256)
//   <dest>\payload\pacote.json      manifesto lido pelo instalador
//   <dest>\payload\reframework\     dinput8.dll do REFramework (opcional) + origem/licenca
//   <dest>\payload\ini\<Exe>.ini    INIs por jogo (opcional)
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

    // args: pacote manifesto helper ini refDir(-) dest versao manterDiag(0/1) modelos [Exe=ini ...]
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
            string texto = comIni ? l1.GetString(File.ReadAllBytes(ini)) : "";
            List<string> mudancas = new List<string>();
            if (comIni && !manterDiag)
            {
                string[] desligar = { "NeuralGravarF9", "NeuralGravarF9Reserva", "NeuralTelaCaptura", "NeuralFgConfigLog", "NeuralDumpFinalMovimento",
                    "NeuralDumpFrames", "NeuralDumpFillStages", "NeuralDumpDepth", "NeuralKernelTrace", "NeuralContractTrace" };
                foreach (string k in desligar)
                {
                    Regex re = new Regex(@"(?m)^(\s*" + k + @"\s*=)([^\r\n]*)");
                    Match m = re.Match(texto);
                    if (m.Success && m.Groups[2].Value.Trim() != "0")
                    {
                        mudancas.Add(k + " " + m.Groups[2].Value.Trim() + " -> 0");
                        texto = re.Replace(texto, "${1}0", 1);
                    }
                }
            }
            if (comIni)
            {
                string iniDest = Path.Combine(mod, "dlssnr_on_amd.ini");
                File.WriteAllBytes(iniDest, l1.GetBytes(texto));
                arquivos["dlssnr_on_amd.ini"] = Sha(iniDest);
            }
            string modelos = a[8];
            for (int i = 9; i < a.Length; i++)
            {
                int eq = a[i].IndexOf('=');
                if (eq <= 0) continue;
                string pastaIni = Path.Combine(payload, "ini");
                Directory.CreateDirectory(pastaIni);
                File.Copy(a[i].Substring(eq + 1), Path.Combine(pastaIni, a[i].Substring(0, eq) + ".ini"));
            }

            Dictionary<string, object> doc = new Dictionary<string, object>();
            doc["produto"] = "AMDLSS5 MOD";
            doc["versao"] = versao;
            doc["montado"] = DateTime.Now.ToString("s");
            doc["arquivos"] = arquivos;
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

            Console.WriteLine("arquivos=" + arquivos.Count);
            Console.WriteLine("helper=" + arquivos["nr_native_adapter.dll"]);
            Console.WriteLine("proxy=" + proxy);
            Console.WriteLine("ini=" + (comIni ? arquivos["dlssnr_on_amd.ini"] : "nenhum (o helper cria o INI da proxy)"));
            Console.WriteLine("ini_mudancas=" + string.Join("; ", mudancas.ToArray()));
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
