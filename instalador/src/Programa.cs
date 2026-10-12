// Ponto de entrada. Sem argumentos abre a janela. Modo silencioso (testes e uso avancado):
//
//   AMDLSS5-Instalador.exe --silencioso --detectar    <pasta ou exe>
//   AMDLSS5-Instalador.exe --silencioso --instalar    <pasta ou exe> [--reframework auto|sim|nao] [--forcar]
//                                       [--nvngx <arquivo>] [--pesos <arquivo>] [--permitir-download]
//   AMDLSS5-Instalador.exe --silencioso --desinstalar <pasta ou exe>
//   opcoes comuns: --payload <pasta> (padrao: payload ao lado do exe), --idioma pt|en|es, --saida <arquivo.txt>
//   janela: --pasta <pasta ou exe> (ja abre com o jogo), --captura <arquivo.png> (salva a janela e fecha)
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
            string[] comValor = { "--detectar", "--instalar", "--desinstalar", "--payload", "--idioma", "--saida", "--pasta", "--captura", "--reframework", "--nvngx", "--pesos" };
            for (int i = 0; i < args.Length; i++)
            {
                if (Array.IndexOf(comValor, args[i].ToLowerInvariant()) >= 0 && i + 1 < args.Length) { a[args[i]] = args[i + 1]; i++; }
                else flags.Add(args[i]);
            }
            string v;
            if (a.TryGetValue("--idioma", out v)) T.Definir(v);
            string pastaPayload = a.TryGetValue("--payload", out v) ? Path.GetFullPath(v) : Payload.PastaPadrao();

            if (!flags.Contains("--silencioso"))
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Janela j = new Janela(pastaPayload);
                if (a.TryGetValue("--pasta", out v)) j.PastaInicial = v;
                if (a.TryGetValue("--captura", out v)) j.CapturaPng = Path.GetFullPath(v);
                Application.Run(j);
                return 0;
            }

            AttachConsole(-1);
            StreamWriter arq = null;
            if (a.TryGetValue("--saida", out v)) { arq = new StreamWriter(v, false, new UTF8Encoding(false)); arq.AutoFlush = true; }
            RelatoConsole r = new RelatoConsole(arq);
            try { return Silencioso(a, flags, pastaPayload, r); }
            catch (ErroInstalador e) { r.Linha("ERRO: " + e.Message); return (int)e.Codigo; }
            catch (Exception e) { r.Linha("ERRO: " + e); return 1; }
            finally { if (arq != null) arq.Dispose(); }
        }

        static int Silencioso(Dictionary<string, string> a, HashSet<string> flags, string pastaPayload, IRelato r)
        {
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
            foreach (string b in j.Bloqueios) r.Linha("BLOQUEIO: " + b);
            foreach (string w in j.Avisos) r.Linha("AVISO: " + w);
            if (modo == "detectar") return j.Bloqueios.Count > 0 ? 2 : 0;

            if (modo == "desinstalar") { Instalacao.Desinstalar(j, r); return Directory.Exists(Path.Combine(j.PastaMod, Util.PastaBackup)) ? 1 : 0; }

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
