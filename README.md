# Cloudy — Launcher per Claude Desktop (Fix Flickering G-Sync)

**Cloudy** è un launcher nativo ultra-leggero (~19 KB, **zero dipendenze**, **0 MB di RAM in background**) per Windows 10/11 che risolve definitivamente il problema dello sfarfallio (flickering) di **Claude Desktop** sui monitor compatibili **G-Sync / FreeSync**.

Quando Claude Desktop si aggiorna tramite Microsoft Store / MSIX, il percorso della cartella in `C:\Program Files\WindowsApps\Claude_<versione>_x64__pzs8sxrjxfjjc\app\claude.exe` cambia ad ogni nuova versione, rompendo i collegamenti manuali. **Cloudy** individua automaticamente in meno di 1 millisecondo l'ultima versione installata di Claude e la avvia con il flag `--disable-gpu-compositing`, senza mostrare alcuna finestra del terminale (`cmd.exe`) e senza bisogno di privilegi di Amministratore.

---

## Utilizzo immediato

1. Scarica **`Cloudy.exe`** (già pronto nella root del repository).
2. Fai **doppio click su `Cloudy.exe`** (oppure fissalo alla Barra delle applicazioni / Start / Desktop).
3. *(Opzionale)* Se vuoi creare automaticamente un collegamento sul Desktop con l'icona ufficiale estratta dalla tua installazione di Claude, fai doppio click su **`Crea-Shortcut-Desktop.bat`**.

Ogni volta che Claude si aggiornerà a una nuova versione, non dovrai più modificare nulla: cliccando sempre su `Cloudy.exe` (o sul suo collegamento) verrà sempre avviata la versione più recente.

---

## Opzioni da riga di comando (opzionali)

- **`Cloudy.exe`** *(default)*: trova l'ultima versione di Claude e la avvia immediatamente con `--disable-gpu-compositing`.
- **`Cloudy.exe --status`**: mostra una finestra informativa con la versione di Claude rilevata e il percorso completo di `claude.exe`.
- **`Cloudy.exe --restart`**: chiude eventuali processi di Claude già aperti in background nella System Tray e riavvia Claude con `--disable-gpu-compositing`.

---

## Come funziona

1. **Ricerca istantanea senza permessi Admin**: poiché Windows blocca la lettura diretta della cartella `C:\Program Files\WindowsApps` agli utenti standard, `Cloudy.exe` interroga direttamente il registro `AppModel\Repository\Packages` di Windows (`HKCU` / `HKLM`) per individuare tutte le versioni del pacchetto `Claude_*__pzs8sxrjxfjjc`, confronta i numeri di versione e seleziona sempre la più recente. Supporta anche le installazioni classiche in `%LOCALAPPDATA%\AnthropicClaude`.
2. **Nessuna finestra CMD visibile**: compilato come eseguibile nativo Win32 GUI (`SUBSYSTEM:WINDOWS`), esegue l'equivalente di `cmd.exe /c start "" "<percorso>\app\claude.exe" --disable-gpu-compositing` con `CREATE_NO_WINDOW` e termina istantaneamente lasciando **zero processi in background**.

---

## Compilazione dal codice sorgente (opzionale)

Il progetto include sia il sorgente nativo in **C (Win32 API pura)** in [`src/cloudy.c`](src/cloudy.c) sia una versione equivalente in **C#** in [`src/Cloudy.cs`](src/Cloudy.cs):

- **Su Windows (senza installare nulla)**: fai doppio click su `Build-Local.bat` (usa il compilatore `csc.exe` preinstallato in ogni copia di Windows 10/11).
- **Cross-compilazione Clang/LLD**: esegui `./scripts/build.sh`.
