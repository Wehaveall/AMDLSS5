// Capas: acha a imagem de capa de cada jogo para a biblioteca, com cache em disco.
// Ordem: arquivo ja no cache -> cache local da Steam -> web da Steam (por appid) -> para jogos de outras lojas, SO com
// BuscarPorNome (--capas-por-nome), busca do nome na loja da Steam para achar o appid. rev-diag (22/10, review M-1): a busca
// mandava o nome de cada jogo de Epic/GOG/Ubisoft/EA/Battle.net/Xbox para a Steam sem o usuario pedir; agora so com a opcao
// (o LEIA-ME diz que as capas vem da Steam). Devolve o caminho do arquivo ou null (a janela desenha a capa simples).
// Nunca lanca: qualquer falha de rede ou de arquivo vira null. Chamar fora da thread da interface.
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;

namespace Amdlss5.Instalador
{
    public sealed class OpcoesCapas
    {
        public string Cache;       // vazio = %LOCALAPPDATA%\AMDLSS5\capas
        public string SteamRaiz;   // vazio = SteamPath do registro
        public bool SemRede;       // so cache e Steam local (testes offline)
        public bool BuscarPorNome; // jogo sem appid da Steam: busca o nome na loja da Steam (--capas-por-nome; padrao nao)
    }

    public static class Capas
    {
        sealed class ClienteTempo : WebClient
        {
            protected override WebRequest GetWebRequest(Uri endereco)
            {
                WebRequest w = base.GetWebRequest(endereco);
                if (w != null)
                {
                    w.Timeout = 10000;
                    HttpWebRequest h = w as HttpWebRequest;
                    if (h != null) h.ReadWriteTimeout = 10000;
                }
                return w;
            }
        }

        public static string PastaCachePadrao()
        {
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "AMDLSS5\\capas");
        }

        public static string Obter(JogoLoja l, OpcoesCapas o)
        {
            try
            {
                if (l == null) return null;
                if (o == null) o = new OpcoesCapas();
                string cache = string.IsNullOrEmpty(o.Cache) ? PastaCachePadrao() : o.Cache;
                string destino = Path.Combine(cache, NomeArquivo(l));
                // 21/10: capa da cache menor que TamanhoMinimo = marcador (a Steam devolve um de 1,6 KB para jogo novo) ou sobra de teste: baixa de novo
                if (File.Exists(destino))
                {
                    if (new FileInfo(destino).Length >= TamanhoMinimo) return destino;
                    try { File.Delete(destino); } catch (Exception) { }
                }
                Directory.CreateDirectory(cache);

                string appid = null;
                if (string.Equals(l.Loja, "steam", StringComparison.OrdinalIgnoreCase) && EhNumero(l.Id)) appid = l.Id;

                if (appid != null)
                {
                    foreach (string local in ProcurarLocalSteam(appid, o.SteamRaiz))
                    {
                        try { if (Guardar(File.ReadAllBytes(local), destino)) return destino; } catch (Exception) { }   // arquivo preso: tenta o proximo
                    }
                }
                if (o.SemRede) return null;
                if (appid == null) { if (!o.BuscarPorNome) return null; appid = BuscarAppid(l.Nome); }
                if (appid == null) return null;

                string[] urls = {
                    "https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/" + appid + "/library_600x900.jpg",
                    "https://steamcdn-a.akamaihd.net/steam/apps/" + appid + "/library_600x900.jpg",
                    "https://shared.akamai.steamstatic.com/store_item_assets/steam/apps/" + appid + "/header.jpg",
                    "https://steamcdn-a.akamaihd.net/steam/apps/" + appid + "/header.jpg" };
                foreach (string u in urls)
                {
                    byte[] b = Baixar(u);
                    if (b != null && Guardar(b, destino)) return destino;
                }
                // jogo novo: as imagens ficam em endereco com hash; a loja diz o do cabecalho (horizontal)
                string cab = CabecalhoDaLoja(appid);
                if (cab != null)
                {
                    byte[] b = Baixar(cab);
                    if (b != null && Guardar(b, destino)) return destino;
                }
            }
            catch (Exception) { }
            return null;
        }

        // <loja>_<id ou hash do nome>.jpg so com [A-Za-z0-9_.-]; se o id teve que ser alterado, soma o hash (ids diferentes nao colidem)
        public static string NomeArquivo(JogoLoja l)
        {
            string loja = Limpa(l.Loja ?? "x");
            string id = l.Id ?? "";
            string limpo = Limpa(id);
            if (id.Length == 0) limpo = Hash(l.Nome ?? "");
            else if (limpo != id) limpo += "-" + Hash(id);
            return loja + "_" + limpo + ".jpg";
        }

        static string Limpa(string s) { return Regex.Replace(s, "[^A-Za-z0-9_.-]", "_"); }

        static string Hash(string s)
        {
            using (MD5 m = MD5.Create())
            {
                byte[] h = m.ComputeHash(Encoding.UTF8.GetBytes(s));
                StringBuilder sb = new StringBuilder();
                for (int i = 0; i < 6; i++) sb.Append(h[i].ToString("x2"));
                return sb.ToString();
            }
        }

        static bool EhNumero(string s) { return !string.IsNullOrEmpty(s) && Regex.IsMatch(s, "^[0-9]{1,10}$"); }

        // sem maiusculas e sem simbolos
        static string Normalizar(string s) { return Regex.Replace((s ?? "").ToLowerInvariant(), "[^\\p{L}\\p{N}]", ""); }

        // capa menor que isso = marcador/sobra (imagem de verdade da Steam: 20-100 KB)
        const int TamanhoMinimo = 4096;

        // Candidatas na cache local da Steam, da melhor para a pior: vertical antiga (<appid>_library_600x900.jpg, <appid>\library_600x900.jpg),
        // vertical do Steam novo (<appid>\<hash>\library_600x900.jpg ou library_capsule.jpg -- 21/10: RE Requiem, BF6, MH Wilds so tem esta)
        // e, por ultimo, a horizontal (library_header.jpg).
        static List<string> ProcurarLocalSteam(string appid, string raiz)
        {
            List<string> r = new List<string>();
            try
            {
                if (string.IsNullOrEmpty(raiz))
                    raiz = Microsoft.Win32.Registry.GetValue("HKEY_CURRENT_USER\\Software\\Valve\\Steam", "SteamPath", null) as string;
                if (string.IsNullOrEmpty(raiz)) return r;
                string lib = Path.Combine(raiz, "appcache\\librarycache");
                string a = Path.Combine(lib, appid + "_library_600x900.jpg");
                if (File.Exists(a)) r.Add(a);
                string pasta = Path.Combine(lib, appid);
                if (Directory.Exists(pasta))
                {
                    string b = Path.Combine(pasta, "library_600x900.jpg");
                    if (File.Exists(b)) r.Add(b);
                    foreach (string nome in new[] { "library_600x900.jpg", "library_capsule.jpg", "library_header.jpg" })
                        foreach (string f in Directory.GetFiles(pasta, nome, SearchOption.AllDirectories)) if (!r.Contains(f)) r.Add(f);
                }
            }
            catch (Exception) { }
            return r;
        }

        // header_image do appdetails da loja (endereco com hash dos jogos novos); null = nao achou
        static string CabecalhoDaLoja(string appid)
        {
            byte[] b = Baixar("https://store.steampowered.com/api/appdetails?appids=" + appid + "&filters=basic");
            if (b == null) return null;
            try
            {
                Dictionary<string, object> raiz = new JavaScriptSerializer().DeserializeObject(Encoding.UTF8.GetString(b)) as Dictionary<string, object>;
                Dictionary<string, object> app = raiz != null && raiz.ContainsKey(appid) ? raiz[appid] as Dictionary<string, object> : null;
                Dictionary<string, object> dados = app != null && app.ContainsKey("data") ? app["data"] as Dictionary<string, object> : null;
                string u = dados != null && dados.ContainsKey("header_image") ? Convert.ToString(dados["header_image"]) : null;
                return !string.IsNullOrEmpty(u) && u.StartsWith("https://", StringComparison.OrdinalIgnoreCase) ? u : null;
            }
            catch (Exception) { return null; }
        }

        static string BuscarAppid(string nome)
        {
            string alvo = Normalizar(nome);
            if (alvo.Length == 0) return null;
            byte[] b = Baixar("https://store.steampowered.com/api/storesearch/?term=" + Uri.EscapeDataString(nome) + "&l=english&cc=US");
            if (b == null) return null;
            try
            {
                Dictionary<string, object> raiz = new JavaScriptSerializer().DeserializeObject(Encoding.UTF8.GetString(b)) as Dictionary<string, object>;
                object[] itens = null;
                if (raiz != null && raiz.ContainsKey("items"))
                {
                    itens = raiz["items"] as object[];
                    if (itens == null) { System.Collections.ArrayList al = raiz["items"] as System.Collections.ArrayList; if (al != null) itens = al.ToArray(); }
                }
                if (itens == null) return null;
                foreach (object it in itens)
                {
                    Dictionary<string, object> d = it as Dictionary<string, object>;
                    if (d == null || !d.ContainsKey("name") || !d.ContainsKey("id")) continue;
                    if (Normalizar(Convert.ToString(d["name"])) != alvo) continue;
                    string id = Convert.ToString(d["id"]);
                    if (EhNumero(id)) return id;
                }
            }
            catch (Exception) { }
            return null;
        }

        static byte[] Baixar(string url)
        {
            try
            {
                try { ServicePointManager.SecurityProtocol |= (SecurityProtocolType)3072; } catch (Exception) { }
                using (ClienteTempo c = new ClienteTempo())
                {
                    c.Headers[HttpRequestHeader.UserAgent] = "AMDLSS5-Instalador";
                    return c.DownloadData(url);
                }
            }
            catch (Exception) { return null; }
        }

        // Confere que os bytes sao uma imagem e grava (arquivo temporario + mover). Falso = descartado.
        static bool Guardar(byte[] bytes, string destino)
        {
            string tmp = destino + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try
            {
                if (bytes == null || bytes.Length < TamanhoMinimo) return false;   // marcador da Steam / imagem vazia
                using (MemoryStream ms = new MemoryStream(bytes))
                using (Image img = Image.FromStream(ms)) { if (img.Width < 100 || img.Height < 100) return false; }
                File.WriteAllBytes(tmp, bytes);
                if (File.Exists(destino)) File.Delete(destino);
                File.Move(tmp, destino);
                return true;
            }
            catch (Exception) { return false; }
            finally { try { if (File.Exists(tmp)) File.Delete(tmp); } catch (Exception) { } }
        }
    }
}
