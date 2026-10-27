// Ponto de entrada. Sem argumentos abre a biblioteca (jogos de todas as lojas); --pasta abre a janela de antes
// ("adicionar jogo manualmente"). Modo silencioso (testes e uso avancado):
//
//   AMDLSS5-Instalador.exe --silencioso --detectar    <pasta ou exe>
//   AMDLSS5-Instalador.exe --silencioso --instalar    <pasta ou exe> [--reframework auto|sim|nao] [--forcar]
//                                       [--nvngx <arquivo>] [--pesos <arquivo>] [--permitir-download]
//   AMDLSS5-Instalador.exe --silencioso --desinstalar <pasta ou exe>
//   AMDLSS5-Instalador.exe --silencioso --listar [--steam <pasta>] [--epic <pasta>] [--xbox <pasta>] [--sem-registro]
//   opcoes comuns: --payload <pasta> (padrao: payload ao lado do exe), --idioma pt|en|es, --saida <arquivo.txt>
//   janela: --pasta <pasta ou exe> (janela de antes, ja com o jogo), --captura <arquivo.png> (salva a janela e fecha)
//   biblioteca: --steam/--epic/--xbox/--sem-registro (lojas), --capas <cache> --sem-rede --capas-por-nome (capas), --idioma, --captura
//
// Codigos de saida: 0 ok, 1 erro, 2 bloqueado/avisos sem --forcar, 3 jogo aberto, 4 sem permissao,
// 5 arquivo faltando (pacote ou NVIDIA).
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace Amdlss5.Instalador
{
    static class Programa
    {
        [DllImport("kernel32.dll")]
        static extern bool AttachConsole(int pid);

        sealed class RelatoConsole : IRelato
        {
            readonly StreamWriter saida;
            public RelatoConsole(StreamWriter saida) { this.saida = saida; }
            public void Linha(string texto)
            {
                try { Console.WriteLine(texto); } catch (Exception) { }
                if (saida != null) saida.WriteLine(texto);
            }
            public void Progresso(int p) { }
        }

        [STAThread]
        static int Main(string[] args)
        {
            Dictionary<string, string> a = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            HashSet<string> flags = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            string[] comValor = { "--detectar", "--instalar", "--desinstalar", "--payload", "--idioma", "--saida", "--pasta", "--captura", "--reframework", "--nvngx", "--pesos", "--steam", "--epic", "--xbox", "--xbox-disco", "--capa", "--capas" };
            for (int i = 0; i < args.Length; i++)
            {
                if (Array.IndexOf(comValor, args[i].ToLowerInvariant()) >= 0 && i + 1 < args.Length) { a[args[i]] = args[i + 1]; i++; }
                else flags.Add(args[i]);
            }
            string v;
            if (a.TryGetValue("--idioma", out v)) T.Definir(v);
            // caminho invalido em --payload/--captura/--saida nao derruba o exe fora de um try: o erro vira mensagem
            string pastaPayload;
            try { pastaPayload = a.TryGetValue("--payload", out v) ? Path.GetFullPath(v) : Payload.PastaPadrao(); }
            catch (Exception) { pastaPayload = v; }   // Payload.Carregar diz que nao achou o pacote

            if (!flags.Contains("--silencioso"))
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                string captura = null;
                if (a.TryGetValue("--captura", out v)) { try { captura = Path.GetFullPath(v); } catch (Exception) { } }
                if (a.TryGetValue("--pasta", out v))
                {
                    // tela de "adicionar jogo manualmente" (a janela de antes), ja com o jogo escolhido
                    Janela j = new Janela(pastaPayload);
                    j.PastaInicial = v;
                    j.CapturaPng = captura;
                    Application.Run(j);
                    return 0;
                }
                Biblioteca b = new Biblioteca(pastaPayload, OpcoesLojasDe(a, flags), OpcoesCapasDe(a, flags));
                b.CapturaPng = captura;
                Application.Run(b);
                return 0;
            }

            AttachConsole(-1);
            StreamWriter arq = null;
            if (a.TryGetValue("--saida", out v))
            {
                try { arq = new StreamWriter(v, false, new UTF8Encoding(false)); arq.AutoFlush = true; }
                catch (Exception e) { try { Console.WriteLine("ERRO: --saida " + v + ": " + e.Message); } catch (Exception) { } return 1; }
            }
            RelatoConsole r = new RelatoConsole(arq);
            try { return Silencioso(a, flags, pastaPayload, r); }
            catch (ErroInstalador e) { r.Linha("ERRO: " + e.Message); return (int)e.Codigo; }
            catch (Exception e) { r.Linha("ERRO: " + e); return 1; }
            finally { if (arq != null) arq.Dispose(); }
        }

        // --listar [--steam <pasta>] [--epic <pasta>] [--xbox <pasta>] [--sem-registro]: uma linha
        // JOGO|loja|id|nome|pasta|grupo|exe|versao_instalada (ou -) por jogo. Pasta sem exe de jogo nao entra na lista.
        static int Listar(Dictionary<string, string> a, HashSet<string> flags, string pastaPayload, IRelato r)
        {
            Payload p = null;
            try { p = Payload.Carregar(pastaPayload); }
            catch (ErroInstalador) { }   // sem pacote (ou pacote ruim) nenhum jogo e "testado"
            foreach (Classificado c in Classificacao.ListarClassificados(OpcoesLojasDe(a, flags), p, r))
            {
                JogoLoja j = c.Loja;
                r.Linha("JOGO|" + j.Loja + "|" + j.Id + "|" + j.Nome.Replace('|', '/') + "|" + j.Pasta + "|" + Classificacao.NomeGrupo(c.Grupo)
                    + "|" + c.ExeNome + "|" + (c.Jogo.VersaoInstalada ?? "-"));
            }
            return 0;
        }

        // --steam/--epic/--xbox/--xbox-disco/--sem-registro (o --listar e a biblioteca)
        static OpcoesLojas OpcoesLojasDe(Dictionary<string, string> a, HashSet<string> flags)
        {
            OpcoesLojas o = new OpcoesLojas();
            string v;
            if (a.TryGetValue("--steam", out v)) o.SteamRaiz = v;
            if (a.TryGetValue("--epic", out v)) o.EpicManifestos = v;
            if (a.TryGetValue("--xbox", out v)) o.XboxRaizes.Add(v);
            if (a.TryGetValue("--xbox-disco", out v)) o.XboxDiscos = new List<string> { v };   // so testes
            o.SemRegistro = flags.Contains("--sem-registro");
            return o;
        }

        // --capas/--steam/--sem-rede (o --capa e a biblioteca)
        static OpcoesCapas OpcoesCapasDe(Dictionary<string, string> a, HashSet<string> flags)
        {
            OpcoesCapas o = new OpcoesCapas { SemRede = flags.Contains("--sem-rede"), BuscarPorNome = flags.Contains("--capas-por-nome") };
            string v;
            if (a.TryGetValue("--capas", out v)) o.Cache = v;
            if (a.TryGetValue("--steam", out v)) o.SteamRaiz = v;
            return o;
        }

        // --capa <loja>|<id>|<nome> --capas <cache> [--steam <pasta>] [--sem-rede]: imprime "CAPA: <caminho|->"
        static int Capa(Dictionary<string, string> a, HashSet<string> flags, IRelato r)
        {
            string[] p = a["--capa"].Split(new char[] { '|' }, 3);
            JogoLoja j = new JogoLoja { Loja = p[0], Id = p.Length > 1 ? p[1] : "", Nome = p.Length > 2 ? p[2] : "" };
            string c = Capas.Obter(j, OpcoesCapasDe(a, flags));
            r.Linha("CAPA: " + (c ?? "-"));
            return 0;
        }

        static int Silencioso(Dictionary<string, string> a, HashSet<string> flags, string pastaPayload, IRelato r)
        {
            if (flags.Contains("--listar")) return Listar(a, flags, pastaPayload, r);
            if (a.ContainsKey("--capa")) return Capa(a, flags, r);
            string alvo;
            string modo = a.TryGetValue("--instalar", out alvo) ? "instalar" : a.TryGetValue("--desinstalar", out alvo) ? "desinstalar" : a.TryGetValue("--detectar", out alvo) ? "detectar" : null;
            if (modo == null) { r.Linha("uso: --silencioso --detectar|--instalar|--desinstalar <pasta ou exe>"); return 1; }

            Payload p = null;
            try { p = Payload.Carregar(pastaPayload); r.Linha("pacote: " + p.Produto + " " + p.Versao + " (" + p.Pasta + ")"); }
            catch (ErroInstalador e) { if (modo == "instalar") throw; r.Linha("pacote: " + e.Message); }

            Jogo j = Detector.Detectar(alvo, p);
            r.Linha("jogo: " + j.Nome);
            r.Linha("exe: " + (j.Exe ?? "-"));
            r.Linha("pasta_mod: " + (j.PastaMod ?? "-"));
            r.Linha("dx12: " + j.Dx12 + "  fsr: " + j.FfxTipo + (j.FfxVersao != null ? " " + j.FfxVersao : "") + "  re_engine: " + j.ReEngine + (j.ReMotivo != null ? " (" + j.ReMotivo + ")" : "") + "  storage: " + j.TemStorage);
            r.Linha("sessoes_backup: " + j.SessoesBackup + "  dinput8: " + (j.Dinput8Sha == null ? "-" : Util.Curto(j.Dinput8Sha)));
            r.Linha("versao_instalada: " + (j.VersaoInstalada ?? "-"));
            foreach (string b in j.Bloqueios) r.Linha("BLOQUEIO: " + b);
            foreach (string w in j.Avisos) r.Linha("AVISO: " + w);
            if (modo == "detectar") return j.Bloqueios.Count > 0 ? 2 : 0;

            // parcial = sobrou sessao com algo a desfazer (uma pasta vazia de efeito que nao saiu nao conta)
            if (modo == "desinstalar") { Instalacao.Desinstalar(j, r); return Sessoes.Resta(j.PastaMod) ? 1 : 0; }

            bool forcar = flags.Contains("--forcar");
            if (j.Bloqueios.Count > 0) return 2;
            if (j.Avisos.Count > 0 && !forcar) { r.Linha("avisos: use --forcar para instalar mesmo assim"); return 2; }
            OpcoesInstalacao o = new OpcoesInstalacao();
            string rf;
            rf = a.TryGetValue("--reframework", out rf) ? rf.ToLowerInvariant() : "auto";
            o.InstalarReframework = rf == "sim" || (rf == "auto" && j.ReEngine);
            o.PermitirDownload = flags.Contains("--permitir-download"); // download do REFramework: so pedido (o pacote ja o traz)
            o.Idioma = a.ContainsKey("--idioma") ? T.Codigo : null;
            string f;
            if (a.TryGetValue("--nvngx", out f)) o.Fornecidos[Util.NomeNvngx] = Path.GetFullPath(f);
            if (a.TryGetValue("--pesos", out f)) o.Fornecidos[Util.NomePesos] = Path.GetFullPath(f);
            Instalacao.Instalar(j, p, o, r);
            return 0;
        }
    }
}
