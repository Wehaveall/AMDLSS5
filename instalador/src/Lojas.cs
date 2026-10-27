// Lojas: acha os jogos instalados em Steam, Epic, GOG, Ubisoft Connect, EA app, Battle.net e Xbox,
// para a biblioteca nunca pedir pasta ao usuario. So le (registro, manifestos, pastas); nada e escrito.
// Cada fonte e independente: se uma falha, vira a linha "LOJA_ERRO: <loja>: <mensagem>" e as outras seguem.
// Duplicatas (mesma pasta por duas lojas) ficam uma vez, na primeira fonte da ordem
// steam, epic, gog, ubisoft, ea, battlenet, xbox.
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Web.Script.Serialization;
using Microsoft.Win32;

namespace Amdlss5.Instalador
{
    public sealed class JogoLoja
    {
        public string Loja;   // steam|epic|gog|ubisoft|ea|battlenet|xbox|manual
        public string Id;
        public string Nome;
        public string Pasta;
    }

    public sealed class OpcoesLojas
    {
        public string SteamRaiz;            // vazio = SteamPath do registro
        public string EpicManifestos;       // vazio = ProgramData\Epic\EpicGamesLauncher\Data\Manifests
        public List<string> XboxRaizes = new List<string>();   // vazio = .GamingRoot dos discos fixos
        public List<string> XboxDiscos;     // so testes: raizes de disco a varrer no lugar dos discos fixos (null = reais)
        public bool SemRegistro;            // nao le o registro nem descobre nada sozinho (testes isolados)
    }

    // Leitor pequeno de VDF/ACF da Valve: "chave" "valor" e "chave" { ... }. Tolerante: lixo ou arquivo
    // truncado devolve o que deu para ler, nunca lanca. Valores sao string; blocos, Dictionary.
    public static class Vdf
    {
        public static Dictionary<string, object> Ler(string texto)
        {
            int i = 0;
            return Bloco(texto, ref i, true);
        }

        static Dictionary<string, object> Bloco(string t, ref int i, bool raiz)
        {
            Dictionary<string, object> d = new Dictionary<string, object>(StringComparer.OrdinalIgnoreCase);
            while (true)
            {
                bool abre, fecha;
                string k = Proximo(t, ref i, out abre, out fecha);
                if (k == null && !abre && !fecha) return d;     // fim do texto
                if (fecha) { if (raiz) continue; return d; }
                if (abre) continue;
                string v = Proximo(t, ref i, out abre, out fecha);
                if (v == null && !abre && !fecha) return d;
                if (abre) d[k] = Bloco(t, ref i, false);
                else if (fecha) { if (raiz) continue; return d; }   // chave sem valor antes do }
                else d[k] = v;
            }
        }

        // Le o proximo token: string entre aspas (\\ \" \n \t desescapados), ou chave solta { / }.
        static string Proximo(string t, ref int i, out bool abre, out bool fecha)
        {
            abre = fecha = false;
            while (i < t.Length)
            {
                char c = t[i];
                if (char.IsWhiteSpace(c)) { i++; continue; }
                if (c == '/' && i + 1 < t.Length && t[i + 1] == '/') { while (i < t.Length && t[i] != '\n') i++; continue; }
                if (c == '{') { i++; abre = true; return null; }
                if (c == '}') { i++; fecha = true; return null; }
                if (c == '"')
                {
                    StringBuilder sb = new StringBuilder();
                    i++;
                    while (i < t.Length && t[i] != '"')
                    {
                        if (t[i] == '\\' && i + 1 < t.Length)
                        {
                            char n = t[i + 1];
                            sb.Append(n == 'n' ? '\n' : n == 't' ? '\t' : n);
                            i += 2;
                        }
                        else sb.Append(t[i++]);
                    }
                    i++;
                    return sb.ToString();
                }
                i++;   // lixo fora das aspas
            }
            return null;
        }

        public static string Texto(Dictionary<string, object> d, string chave)
        {
            object o;
            return d != null && d.TryGetValue(chave, out o) ? o as string : null;
        }

        public static Dictionary<string, object> Sub(Dictionary<string, object> d, string chave)
        {
            object o;
            return d != null && d.TryGetValue(chave, out o) ? o as Dictionary<string, object> : null;
        }
    }

    public static class Lojas
    {
        static readonly string[] FerramentasSteam = { "228980", "1070560", "1391110", "1628350" };

        delegate void LeFonte(List<JogoLoja> l);

        public static List<JogoLoja> Listar(OpcoesLojas o, IRelato r)
        {
            if (o == null) o = new OpcoesLojas();
            List<JogoLoja> todos = new List<JogoLoja>();
            Fonte("steam", todos, r, delegate(List<JogoLoja> l) { Steam(o, l, r); });
            Fonte("epic", todos, r, delegate(List<JogoLoja> l) { Epic(o, l); });
            Fonte("gog", todos, r, delegate(List<JogoLoja> l) { if (!o.SemRegistro) Gog(l); });
            Fonte("ubisoft", todos, r, delegate(List<JogoLoja> l) { if (!o.SemRegistro) Ubisoft(l); });
            Fonte("ea", todos, r, delegate(List<JogoLoja> l) { if (!o.SemRegistro) Desinstalacao(l, "ea", "Electronic Arts", new[] { "EA app", "EA Desktop", "Origin" }); });
            Fonte("battlenet", todos, r, delegate(List<JogoLoja> l) { if (!o.SemRegistro) Desinstalacao(l, "battlenet", "Blizzard Entertainment", new[] { "Battle.net" }); });
            Fonte("xbox", todos, r, delegate(List<JogoLoja> l) { Xbox(o, l); });

            List<JogoLoja> saida = new List<JogoLoja>();
            HashSet<string> vistos = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (JogoLoja j in todos)
            {
                string chave;
                try { chave = Path.GetFullPath(j.Pasta).TrimEnd('\\', '/'); } catch (Exception) { continue; }
                if (!Directory.Exists(chave)) continue;   // pasta que nao existe = fora
                if (!vistos.Add(chave)) continue;
                j.Pasta = chave;
                saida.Add(j);
            }
            return saida;
        }

        static void Fonte(string loja, List<JogoLoja> todos, IRelato r, LeFonte f)
        {
            List<JogoLoja> l = new List<JogoLoja>();
            try { f(l); }
            catch (Exception e) { r.Linha("LOJA_ERRO: " + loja + ": " + e.Message); }
            todos.AddRange(l);   // o que foi lido antes da falha vale
        }

        static void Add(List<JogoLoja> l, string loja, string id, string nome, string pasta)
        {
            if (string.IsNullOrWhiteSpace(pasta)) return;
            if (string.IsNullOrWhiteSpace(nome)) nome = Path.GetFileName(pasta.TrimEnd('\\', '/'));
            l.Add(new JogoLoja { Loja = loja, Id = id ?? "", Nome = nome, Pasta = pasta });
        }

        // ---------------------------------------------------------------- Steam
        static void Steam(OpcoesLojas o, List<JogoLoja> l, IRelato r)
        {
            string raiz = o.SteamRaiz;
            if (string.IsNullOrEmpty(raiz) && !o.SemRegistro)
            {
                using (RegistryKey k = Registry.CurrentUser.OpenSubKey(@"Software\Valve\Steam"))
                    raiz = k == null ? null : k.GetValue("SteamPath") as string;
            }
            if (string.IsNullOrEmpty(raiz)) return;
            raiz = Path.GetFullPath(raiz.Replace('/', '\\'));   // o registro guarda com barras normais

            List<string> bibliotecas = new List<string>();
            bibliotecas.Add(raiz);
            string vdf = Path.Combine(raiz, @"steamapps\libraryfolders.vdf");
            if (File.Exists(vdf))
            {
                try
                {
                    Dictionary<string, object> blocos = Vdf.Sub(Vdf.Ler(File.ReadAllText(vdf)), "libraryfolders");
                    if (blocos != null)
                        foreach (KeyValuePair<string, object> kv in blocos)
                        {
                            string p = kv.Value as string;   // formato antigo: "1" "D:\\Pasta"
                            Dictionary<string, object> b = kv.Value as Dictionary<string, object>;
                            if (b != null) p = Vdf.Texto(b, "path");   // formato novo
                            if (!string.IsNullOrEmpty(p) && p.Length > 2 && ((char.IsLetter(p[0]) && p[1] == ':') || p.StartsWith("\\\\")))
                                bibliotecas.Add(p);
                        }
                }
                catch (Exception e) { r.Linha("LOJA_ERRO: steam: libraryfolders.vdf: " + e.Message); }
            }

            HashSet<string> vistas = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (string bib in bibliotecas)
            {
                string apps;
                try { apps = Path.Combine(Path.GetFullPath(bib), "steamapps"); } catch (Exception) { continue; }
                if (!vistas.Add(apps) || !Directory.Exists(apps)) continue;
                foreach (string acf in Directory.GetFiles(apps, "appmanifest_*.acf"))
                {
                    try
                    {
                        Dictionary<string, object> s = Vdf.Sub(Vdf.Ler(File.ReadAllText(acf)), "AppState");
                        string id = Vdf.Texto(s, "appid"), nome = Vdf.Texto(s, "name"), dir = Vdf.Texto(s, "installdir");
                        if (string.IsNullOrEmpty(id) || string.IsNullOrEmpty(dir)) { r.Linha("LOJA_ERRO: steam: " + Path.GetFileName(acf) + ": manifesto invalido"); continue; }
                        // rev-diag (22/10, review M-14): installdir e o nome de UMA pasta em steamapps\common (a regra das chaves do payload):
                        // um .acf adulterado com .. / \ / : apontava o cartao para qualquer pasta do disco.
                        if (dir.IndexOfAny(new char[] { '\\', '/', ':' }) >= 0 || dir.Contains("..")) { r.Linha("LOJA_ERRO: steam: " + Path.GetFileName(acf) + ": installdir invalido"); continue; }
                        if (Array.IndexOf(FerramentasSteam, id) >= 0) continue;
                        if (nome != null && (nome.IndexOf("Proton", StringComparison.OrdinalIgnoreCase) >= 0 || nome.IndexOf("Redistributable", StringComparison.OrdinalIgnoreCase) >= 0)) continue;
                        Add(l, "steam", id, nome, Path.Combine(apps, "common", dir));
                    }
                    catch (Exception e) { r.Linha("LOJA_ERRO: steam: " + Path.GetFileName(acf) + ": " + e.Message); }
                }
            }
        }

        // ---------------------------------------------------------------- Epic
        static void Epic(OpcoesLojas o, List<JogoLoja> l)
        {
            string dir = o.EpicManifestos;
            if (string.IsNullOrEmpty(dir))
            {
                if (o.SemRegistro) return;
                dir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), @"Epic\EpicGamesLauncher\Data\Manifests");
            }
            if (!Directory.Exists(dir)) return;
            foreach (string f in Directory.GetFiles(dir, "*.item"))
            {
                try
                {
                    Dictionary<string, object> d = new JavaScriptSerializer().DeserializeObject(File.ReadAllText(f, Encoding.UTF8)) as Dictionary<string, object>;
                    if (d == null) continue;
                    Add(l, "epic", Cad(d, "CatalogItemId") ?? Cad(d, "AppName") ?? Path.GetFileNameWithoutExtension(f), Cad(d, "DisplayName"), Cad(d, "InstallLocation"));
                }
                catch (Exception) { }   // um .item corrompido nao derruba os outros
            }
        }

        static string Cad(Dictionary<string, object> d, string k)
        {
            object v;
            return d.TryGetValue(k, out v) ? v as string : null;
        }

        // ---------------------------------------------------------------- GOG, Ubisoft, EA, Battle.net (registro)
        static RegistryKey Abre(RegistryHive h, RegistryView v, string caminho)
        {
            return RegistryKey.OpenBaseKey(h, v).OpenSubKey(caminho);
        }

        static void Gog(List<JogoLoja> l)
        {
            foreach (RegistryView v in new[] { RegistryView.Registry32, RegistryView.Registry64 })
                using (RegistryKey k = Abre(RegistryHive.LocalMachine, v, @"SOFTWARE\GOG.com\Games"))
                {
                    if (k == null) continue;
                    foreach (string id in k.GetSubKeyNames())
                        using (RegistryKey g = k.OpenSubKey(id))
                            if (g != null) Add(l, "gog", id, g.GetValue("gameName") as string, g.GetValue("path") as string);
                }
        }

        static void Ubisoft(List<JogoLoja> l)
        {
            foreach (RegistryView v in new[] { RegistryView.Registry32, RegistryView.Registry64 })
                using (RegistryKey k = Abre(RegistryHive.LocalMachine, v, @"SOFTWARE\Ubisoft\Launcher\Installs"))
                {
                    if (k == null) continue;
                    foreach (string id in k.GetSubKeyNames())
                        using (RegistryKey g = k.OpenSubKey(id))
                        {
                            string dir = g == null ? null : g.GetValue("InstallDir") as string;
                            if (!string.IsNullOrEmpty(dir)) Add(l, "ubisoft", id, null, dir.Replace('/', '\\'));
                        }
                }
        }

        // EA app e Battle.net nao tem lista propria estavel: usam as chaves de desinstalacao do Windows, filtradas pelo Publisher.
        static void Desinstalacao(List<JogoLoja> l, string loja, string publisher, string[] launchers)
        {
            foreach (RegistryHive h in new[] { RegistryHive.LocalMachine, RegistryHive.CurrentUser })
                foreach (RegistryView v in new[] { RegistryView.Registry32, RegistryView.Registry64 })
                    using (RegistryKey k = Abre(h, v, @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"))
                    {
                        if (k == null) continue;
                        foreach (string id in k.GetSubKeyNames())
                            using (RegistryKey g = k.OpenSubKey(id))
                            {
                                if (g == null) continue;
                                string pub = g.GetValue("Publisher") as string, loc = g.GetValue("InstallLocation") as string, nome = g.GetValue("DisplayName") as string;
                                if (pub == null || pub.IndexOf(publisher, StringComparison.OrdinalIgnoreCase) < 0 || string.IsNullOrEmpty(loc)) continue;
                                bool launcher = false;
                                foreach (string n in launchers) if (nome != null && nome.StartsWith(n, StringComparison.OrdinalIgnoreCase)) launcher = true;
                                if (!launcher) Add(l, loja, id, nome, loc.Trim('"'));
                            }
                    }
        }

        // ---------------------------------------------------------------- Xbox / Microsoft Store
        static void Xbox(OpcoesLojas o, List<JogoLoja> l)
        {
            // pasta dada pelo usuario: a propria XboxGames, ou uma raiz com XboxGames dentro
            foreach (string x in o.XboxRaizes)
            {
                if (string.IsNullOrEmpty(x)) continue;
                string xg = Path.Combine(x, "XboxGames");
                XboxJogos(Directory.Exists(xg) ? xg : x, l);
            }
            // descoberta: so discos com .GamingRoot E pasta XboxGames; sem ela o disco fica de fora
            if (o.XboxRaizes.Count == 0 && (!o.SemRegistro || o.XboxDiscos != null))
            {
                List<string> discos = new List<string>();
                if (o.XboxDiscos != null) discos.AddRange(o.XboxDiscos);
                else
                    foreach (DriveInfo d in DriveInfo.GetDrives())
                        if (d.DriveType == DriveType.Fixed && d.IsReady) discos.Add(d.RootDirectory.FullName);
                foreach (string d in discos) XboxDoDisco(d, l);
            }
        }

        static void XboxDoDisco(string raiz, List<JogoLoja> l)
        {
            if (!File.Exists(Path.Combine(raiz, ".GamingRoot"))) return;
            string xg = Path.Combine(raiz, "XboxGames");
            if (Directory.Exists(xg)) XboxJogos(xg, l);
        }

        static void XboxJogos(string xboxGames, List<JogoLoja> l)
        {
            if (!Directory.Exists(xboxGames)) return;
            foreach (string jogo in Directory.GetDirectories(xboxGames))
            {
                string nome = Path.GetFileName(jogo);
                string conteudo = Path.Combine(jogo, "Content");   // o jogo mora em <nome>\Content
                Add(l, "xbox", nome, nome, Directory.Exists(conteudo) ? conteudo : jogo);
            }
        }
    }
}
