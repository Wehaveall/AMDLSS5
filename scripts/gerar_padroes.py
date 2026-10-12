"""Gera runtime/nr_publico_padroes.h (padroes embutidos do helper) a partir de um INI completo.

Uso: py -3 scripts/gerar_padroes.py --ini <INI> --saida <nr_publico_padroes.h>
"""
import argparse
import re
from pathlib import Path

PROXY = ["Enabled", "LocalTone", "LocalStructure", "SkinStructure", "UseAutoMask", "ToneChannels", "Scale",
         "Temporal", "Tonemap", "UseFsrInputs", "UseDepth", "Interop", "Inline", "InlineWaitMs", "HipDevice",
         "PredWait", "PredSlice"]


DIAGNOSTICO = {
    "NeuralFgConfigLog": "0", "NeuralTelaCaptura": "0", "NeuralTelaCapturaEm": "0", "NeuralGravarF9": "0",
    "NeuralGravarAviso": "0", "NeuralGravarF9Reserva": "0", "NeuralDumpFrames": "0", "NeuralDumpFillStages": "0",
    "NeuralDumpDepth": "0", "NeuralDumpFinalMovimento": "0", "NeuralStyleDiagnostic": "0", "NeuralKernelTrace": "0",
    "NeuralContractTrace": "0", "NeuralContractTraceCopy": "0", "NeuralContractTraceImmediate": "0",
    "NeuralContractTraceFullTensors": "0", "NeuralContractTraceSingle": "0", "NeuralFgTempo": "0",
    "NeuralKernelTimeline": "0", "NeuralKernelTimelineFence": "0", "NeuralMeasurement": "0",
    "NeuralMeasurementClock": "0", "NeuralEsperaDiag": "0", "NeuralCapturaCarimbo": "0", "NeuralCaptureCount": "0",
    "NeuralCenaTrace": "0", "NeuralConv2LaunchTrace": "0", "NeuralHostCopyLog": "0", "NeuralHostGapLog": "0",
    "NeuralReprojectDebugReason": "0", "NeuralCustoPerfil": "0",
}

PUBLICO = {"NeuralPostPercent": "75", "NeuralMenuCompacto": "2", "NeuralMenuAutoFecha": "5",

           "NeuralPassConfig": "769", "NeuralOverlayAmd": "0", "NeuralOverlayAmdRecria": "0",
           "NeuralOverlayAmdRiscoDvr": "0", "NeuralCustoReduzirCache": "1", "NeuralCustoCromaLds": "1",


           "NeuralRedeCadaQuadros": "4",


           "NeuralRostoJogador": "1024", "NeuralRostoJogadorFora": "1", "NeuralRostoJogadorDetThread": "1",
           "NeuralRostoJogadorThreadRede": "1",
           "NeuralRostoJogadorCorte": "27", "NeuralRostoJogadorZoom": "23",

           "NeuralPassoExtraContinuoEscala": "33", "NeuralPassoExtraContinuoForca": "1.0"}


PROXY_PUBLICO = {"Enabled": "0", "PredSlice": "8192"}

FORA = {"NeuralDumpGain", "NeuralDumpAfterFrames", "NeuralDumpEveryFrames", "NeuralContractTraceBytes",
        "NeuralKernelTraceJobs", "NeuralLogTetoMB", "NeuralGravarF9Quadros", "NeuralAgeLogFrames", "NeuralGravarTeto",
        "NeuralCaptureKernel", "NeuralCaptureArgBytes", "NeuralCaptureSkip", "NeuralCaptureMode",
        "NeuralConv2LaunchTraceLimit", "NeuralMeasurementFamily"}

POR_JOGO = {"Cyberpunk2077.exe": {"NeuralSementeFixa": "1", "NeuralRostoJogador": "0"}}
PROXY_CYBERPUNK = {"Temporal": "0"}


def ini_minimo(valores, troca=None):
    troca = {**PROXY_PUBLICO, **(troca or {})}
    linhas = ["[DlssNrOnAmd]"]
    for k in PROXY:
        if k in valores or k in troca:
            v = troca.get(k, "0" if k == "Inline" else valores.get(k))
            linhas.append(f"{k}={v}")
    return linhas


def ler_ini(caminho: Path):
    texto = caminho.read_bytes().decode("utf-8-sig", errors="replace")
    secao, valores, ordem = None, {}, []
    for linha in texto.splitlines():
        linha = linha.strip()
        if not linha or linha[0] in ";#":
            continue
        if linha.startswith("["):
            secao = linha.strip("[]").strip().lower()
            continue
        if secao != "dlssnronamd" or "=" not in linha:
            continue
        k, v = linha.split("=", 1)
        k, v = k.strip(), v.strip()
        if k not in valores:
            ordem.append(k)
        valores[k] = v
    return valores, ordem


def literal(v: str) -> str:
    if not re.fullmatch(r"[0-9A-Za-z.+\-_]*", v):
        raise SystemExit(f"valor inesperado no INI: {v!r}")
    return 'L"' + v + '"'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ini", required=True)
    ap.add_argument("--saida", required=True)
    ap.add_argument("--ini-minimo")
    ap.add_argument("--ini-cyberpunk")
    ap.add_argument("--ini-efetivo")
    a = ap.parse_args()
    valores, ordem = ler_ini(Path(a.ini))
    proxy = {k.lower() for k in PROXY}
    fixo = {**DIAGNOSTICO, **PUBLICO}
    itens = []
    for k in ordem:
        if k.lower() in proxy or k in FORA:
            continue
        itens.append((k, None, fixo.get(k, valores[k])))
    vistos = {k.lower() for k in ordem}
    for k, v in fixo.items():
        if k.lower() not in vistos:
            itens.append((k, None, v))
    for jogo, chaves in POR_JOGO.items():
        for k, v in chaves.items():
            itens.append((k, jogo, v))
    gerais = [k.lower() for k, j, _ in itens if j is None]
    if len(gerais) != len(set(gerais)):
        raise SystemExit("chave repetida")
    corpo = ",\n".join(
        f'    {{L"{k}",{("L" + chr(34) + j + chr(34)) if j else "nullptr"},{literal(v)}}}' for k, j, v in itens)


    proxy_itens = [(k, None, v) for k, v in (l.split("=", 1) for l in ini_minimo(valores)[1:])]
    proxy_itens += [(k, "Cyberpunk2077.exe", v) for k, v in PROXY_CYBERPUNK.items()]
    if len(proxy_itens) != len(PROXY) + len(PROXY_CYBERPUNK):
        raise SystemExit(f"chaves da proxy: {len(proxy_itens)}")
    corpo_proxy = ",\n".join(
        f'    {{L"{k}",{("L" + chr(34) + j + chr(34)) if j else "nullptr"},{literal(v)}}}' for k, j, v in proxy_itens)
    saida = (
        "#pragma once\n"
        "// Gerado por scripts/publico/gerar_padroes.py a partir do INI do RE do autor. Nao editar a mao.\n"
        "// Padroes do helper publico: chave, jogo (exe; nullptr = todos) e valor.\n"
        "namespace nrcfg::padroes {\n"
        "struct Item {const wchar_t* chave;const wchar_t* jogo;const wchar_t* valor;};\n"
        f"inline constexpr Item itens[]={{\n{corpo}\n}};\n"
        "// Chaves que a proxy le do INI: o helper publico grava as que faltarem (o pacote nao leva INI).\n"
        f"inline constexpr Item proxy[]={{\n{corpo_proxy}\n}};\n"
        "}\n"
    )
    Path(a.saida).write_text(saida, encoding="utf-8", newline="\n")
    print(f"PADROES {len(itens)} itens + {len(proxy_itens)} da proxy -> {a.saida}")
    if a.ini_minimo:
        Path(a.ini_minimo).write_bytes(("\r\n".join(ini_minimo(valores)) + "\r\n").encode("ascii"))
        print(f"INI_MINIMO {a.ini_minimo}")
    if a.ini_cyberpunk:
        Path(a.ini_cyberpunk).write_bytes(("\r\n".join(ini_minimo(valores, PROXY_CYBERPUNK)) + "\r\n").encode("ascii"))
        print(f"INI_CYBERPUNK {a.ini_cyberpunk}")
    if a.ini_efetivo:
        linhas = ini_minimo(valores) + [f"{k}={v}" for k, j, v in itens if j is None]
        Path(a.ini_efetivo).write_bytes(("\r\n".join(linhas) + "\r\n").encode("ascii"))
        print(f"INI_EFETIVO {a.ini_efetivo}")


if __name__ == "__main__":
    main()
