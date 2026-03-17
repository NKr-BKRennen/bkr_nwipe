# WYPE

Angepasste Version von [nwipe](https://github.com/martijnvanbrummelen/nwipe) mit eigenem Branding und erweiterten Funktionen.

Basiert auf nwipe (Fork von `dwipe` / Darik's Boot and Nuke) mit folgenden Erweiterungen:

- **Wype-Branding**: Logo, angepasstes PDF-Zertifikat-Layout, modernisierte GUI
- **PDF-Zertifikate**: Hostname, Inventarnummer, Organisation, Kunde auf dem Zertifikat
- **Secure Erase / Sanitize**: Hardware-basierte Lösch-Methoden für ATA, NVMe und SCSI (inkl. SSDs)
- **Per-Disk Metadaten**: Hostname und Inventarnummer pro Festplatte direkt in der GUI eingeben
- **E-Mail-Versand**: Gesammelter Versand aller PDF-Zertifikate per SMTP nach Bestätigung
- **Hilfe & Changelog**: Direkt in der GUI abrufbar (`h` und `l`)

> **Für ein bootbares USB/ISO-Image** das direkt in Wype startet (ohne installiertes OS): [wypeOS](https://github.com/NKr-BKRennen/wypeOS)

---

## Typischer Workflow

So sieht ein typischer Lösch-Vorgang mit Wype aus:

1. **Wype starten** → `sudo wype`
2. **Lösch-Methode wählen** → `m` drücken → z.B. "DoD 5220.22-M" für HDDs oder "Secure Erase / Sanitize >" für SSDs
3. **Optionen anpassen** (optional) → `v` Verifikation, `r` Durchläufe, `p` PRNG, `b` Blanking
4. **Festplatten auswählen** → Pfeiltasten + `Space` (oder `Ctrl+A` für alle)
5. **Metadaten eingeben** → `e` auf jeder Festplatte drücken → Hostname und Inventarnummer eintragen
6. **Wipe starten** → `S` (Shift+S) — Wype warnt automatisch wenn Metadaten fehlen
7. **Warten** → Fortschritt wird live angezeigt. Bei aktivem E-Mail-Versand wird eine Benachrichtigung gesendet wenn alle Wipes fertig sind.
8. **Bestätigen** → `Enter` drücken → PDFs werden erstellt und per E-Mail versendet

> **Tipp:** `h` zeigt jederzeit eine Hilfe-Seite mit allen Tastenbelegungen. `l` zeigt den Changelog.

---

## Konfiguration

Wype speichert alle Einstellungen in `/etc/wype/wype.conf` (libconfig-Format).
Die Datei wird beim **ersten Start** automatisch mit Standardwerten erstellt.

Es gibt **drei Wege** Einstellungen zu ändern:

| Weg | Was | Wann |
|-----|-----|------|
| **GUI-Config-Menü** (`c`-Taste) | Organisation, Kunde, PDF-Optionen | Zur Laufzeit, wird in `wype.conf` gespeichert |
| **GUI-Einstellungen** (Tasten `m`, `v`, `r`, `p`, `b`, `d`) | Lösch-Methode, Verifikation, Durchläufe etc. | Zur Laufzeit, nur für aktuelle Sitzung |
| **Direkt in `/etc/wype/wype.conf` editieren** | Alles inkl. E-Mail-Einstellungen | Vor dem Start, persistent |

### Organisations-Details

Diese Angaben erscheinen auf jedem PDF-Lösch-Zertifikat. **Vor dem ersten Einsatz konfigurieren:**

```
Organisation_Details: {
  Business_Name = "Meine Firma GmbH"
  Business_Address = "Musterstr. 1, 12345 Berlin"
  Contact_Name = "Max Mustermann"
  Contact_Phone = "+49 30 123456"
  Op_Tech_Name = "Techniker Name"
}
```

> Konfigurierbar über **c** → "PDF Report - Edit Organisation" in der GUI.

### Kunden-Zuordnung

Optional kann ein Kunde zugeordnet werden, der auf dem Zertifikat erscheint:

```
Selected_Customer: {
  Customer_Name = "Kunde AG"
  Customer_Address = "Kundenweg 5, 54321 Hamburg"
  Contact_Name = "Ansprechpartner"
  Contact_Phone = "+49 40 654321"
}
```

> Kunden können über **c** → "PDF Report - Select/Add/Delete Customer" verwaltet werden.
> Kundendaten liegen in `/etc/wype/wype_customers.csv`.

### PDF-Zertifikat

```
PDF_Certificate: {
  PDF_Enable = "ENABLED"
  PDF_Preview = "DISABLED"
  PDF_Host_Visibility = "DISABLED"
  PDF_tag = "DISABLED"
  User_Defined_Tag = "Empty Tag"
}
```

| Einstellung | Beschreibung |
|-------------|-------------|
| `PDF_Enable` | PDF-Zertifikate nach Wipe erstellen (`ENABLED`/`DISABLED`) |
| `PDF_Preview` | PDF nach Erstellung öffnen (nur mit Desktop) |
| `PDF_Host_Visibility` | System-Hostname auf dem Zertifikat anzeigen |
| `PDF_tag` | Benutzerdefinierten Tag auf dem Zertifikat anzeigen |
| `User_Defined_Tag` | Freitext-Tag für das Zertifikat |

### Per-Disk Metadaten (Hostname / Inventarnummer)

Pro Festplatte können individuelle Werte gesetzt werden, die auf dem jeweiligen PDF-Zertifikat erscheinen:

1. Festplatte mit Pfeiltasten fokussieren
2. **e** drücken → Hostname und Inventarnummer eingeben (Tab wechselt zwischen Feldern) → Enter

> Diese Werte werden **nicht** in `wype.conf` gespeichert, sondern nur während der Laufzeit gehalten und ins PDF geschrieben.
> Beim Wipe-Start warnt Wype automatisch, wenn bei einer ausgewählten Festplatte Metadaten fehlen.

### E-Mail-Versand (SMTP)

Nach Abschluss aller Wipes werden die PDF-Zertifikate gesammelt per E-Mail versendet:

1. Alle Wipes fertig → Benachrichtigungs-E-Mail ("Wipe fertig, bitte am Gerät bestätigen")
2. Benutzer drückt Enter → PDFs werden erstellt und in **einer** E-Mail gesendet
3. Bei Erfolg: lokale PDFs werden gelöscht. Bei Fehler: PDFs bleiben lokal erhalten.

Der E-Mail-Status wird im Options-Fenster angezeigt (grün "Aktiv" / rot "Deaktiviert").

```
Email_Settings: {
  Email_Enable = "ENABLED"
  SMTP_Server = "mailserver.example.com"
  SMTP_Port = "25"
  Sender_Address = "wype@example.com"
  Recipient_Address = "it-team@example.com"
}
```

| Einstellung | Beschreibung |
|-------------|-------------|
| `Email_Enable` | E-Mail-Versand aktivieren (`ENABLED`/`DISABLED`) |
| `SMTP_Server` | Hostname oder IP des SMTP-Servers |
| `SMTP_Port` | SMTP-Port (Standard: `25`) |
| `Sender_Address` | Absender-Adresse |
| `Recipient_Address` | Empfänger-Adresse (leer = kein Versand) |

> Standardmäßig deaktiviert. Unterstützt SMTP ohne Authentifizierung (Port 25, internes Netz).
> E-Mail-Einstellungen müssen direkt in `/etc/wype/wype.conf` editiert werden (kein GUI-Menü dafür).

---

## Installation auf Debian 13 (Trixie)

### 1. System vorbereiten

```bash
sudo apt update && sudo apt upgrade -y
```

### 2. Abhängigkeiten installieren

```bash
sudo apt install -y \
  build-essential \
  pkg-config \
  automake \
  autoconf \
  libtool \
  git \
  libncurses-dev \
  libparted-dev \
  libconfig-dev \
  libconfig++-dev \
  dmidecode \
  coreutils \
  smartmontools \
  hdparm
```

### 3. Repository klonen und kompilieren

```bash
cd /root
git clone https://github.com/NKr-BKRennen/wype.git
cd wype
chmod +x build.sh
./build.sh
```

Alternativ manuell:

```bash
./autogen.sh
./configure
make -j$(nproc)
sudo make install
```

### 4. Konfiguration anpassen

Beim ersten Start wird `/etc/wype/wype.conf` mit Standardwerten erstellt. Danach anpassen:

```bash
sudo nano /etc/wype/wype.conf
```

Oder in der GUI über die **c**-Taste (Organisation, Kunde, PDF-Einstellungen).

### 5. Starten

```bash
sudo wype
```

### 6. Autostart einrichten (optional)

Damit wype beim Booten automatisch auf tty1 startet (z.B. für dedizierte Lösch-Stationen):

**Auto-Login für root auf tty1 aktivieren:**

```bash
sudo mkdir -p /etc/systemd/system/getty@tty1.service.d
sudo tee /etc/systemd/system/getty@tty1.service.d/override.conf > /dev/null << 'EOF'
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin root --noclear %I $TERM
EOF
```

**wype beim Login automatisch starten:**

```bash
cat >> /root/.bash_profile << 'PROFILE'

# WYPE Autostart auf tty1
if [[ "$(tty)" == "/dev/tty1" ]]; then
    wype
fi
PROFILE
```

**System neustarten:**

```bash
sudo systemctl daemon-reload
sudo reboot
```

### 7. Aktualisieren

```bash
cd /root/wype
./update.sh
```

---

## Schnellinstallation (Copy-Paste)

Alles in einem Block für eine frische Debian 13 Installation:

```bash
sudo apt update && sudo apt install -y \
  build-essential pkg-config automake autoconf libtool git \
  libncurses-dev libparted-dev libconfig-dev libconfig++-dev \
  dmidecode coreutils smartmontools hdparm && \
cd /root && \
git clone https://github.com/NKr-BKRennen/wype.git && \
cd wype && chmod +x build.sh && ./build.sh && \
echo "Installation abgeschlossen. Starten mit: sudo wype"
```

---

## Lösch-Methoden

### Software-basierte Methoden

| Methode | Beschreibung | Pässe |
|---------|-------------|--------|
| Fill With Zeros | Füllt mit Nullen (`0x00`) | 1 |
| Fill With Ones | Füllt mit Einsen (`0xFF`) | 1 |
| RCMP TSSIT OPS-II | Royal Canadian Mounted Police Standard | 8 |
| DoD Short | US DoD 5220.22-M (kurz) | 3 |
| DoD 5220.22-M | US DoD 5220.22-M (voll) — **empfohlen für HDDs** | 7 |
| Gutmann Wipe | Peter Gutmann 35-Pass Methode | 35 |
| PRNG Stream | Zufallsdaten vom gewählten PRNG | 1 |
| HMG IS5 Enhanced | UK HMG IS5 (Enhanced) | 3 |
| Schneier Wipe | Bruce Schneier 7-Pass Methode | 7 |
| BMB21-2019 | Chinesischer Standard für Datensanitisierung | 6 |

### Hardware-basierte Methoden (Secure Erase / Sanitize)

Diese Methoden arbeiten auf Firmware-Ebene und erreichen auch versteckte/reservierte SSD-Bereiche.
**Empfohlen für SSDs/NVMe** — Software-Überschreiben ist bei Flash-Speicher nicht zuverlässig.

| Methode | CLI-Flag | Beschreibung |
|---------|----------|-------------|
| Secure Erase | `--method=secure_erase` | ATA/NVMe Secure Erase + Zero-Verifikation |
| Secure Erase + PRNG | `--method=secure_erase_prng` | Secure Erase + PRNG-Pass + Verifikation |
| Sanitize Crypto Erase | `--method=sanitize_crypto` | Zerstört den Encryption Key — **empfohlen für SSDs** |
| Sanitize Block Erase | `--method=sanitize_block` | Block Erase (NVMe/SCSI) |
| Sanitize Overwrite | `--method=sanitize_overwrite` | Sanitize Overwrite (SCSI) |

Verfügbar über **GUI**: `m` → "Secure Erase / Sanitize >" im Methoden-Menü.

> **Voraussetzungen:** `hdparm` (ATA), `nvme-cli` (NVMe), `sg3_utils` (SCSI)

---

## GUI-Bedienung

### Tastenbelegung (Hauptbildschirm)

| Taste | Funktion |
|-------|----------|
| **Space** | Festplatte auswählen/abwählen |
| **S** | Wipe starten (Shift+S) — prüft fehlende Metadaten |
| **e** | Hostname/Inventarnummer für fokussierte Festplatte bearbeiten |
| **m** | Lösch-Methode wählen |
| **p** | PRNG wählen |
| **v** | Verifikation einstellen |
| **r** | Anzahl Durchläufe |
| **b** | Blanking ein/aus |
| **d** | Schreibrichtung |
| **c** | Konfiguration (Organisation, Kunde, PDF) |
| **t** | Details zur fokussierten Festplatte |
| **h** | Hilfe anzeigen |
| **l** | Changelog anzeigen |
| **Ctrl+A** | Alle Festplatten auswählen |
| **Ctrl+C** | Beenden |

### Config-Menü (`c`-Taste)

Über die **c**-Taste öffnet sich das Konfigurationsmenü mit folgenden Optionen:

- **PDF Report - Edit Organisation** → Firmenname, Adresse, Ansprechpartner, Techniker
- **PDF Report - Select/Add/Delete Customer** → Kundenverwaltung für das Zertifikat
- **PDF Report - Enable/Disable PDF** → PDF-Erstellung an/aus
- **PDF Report - Preview** → PDF nach Erstellung öffnen
- **PDF Report - Host Visibility** → System-Hostname auf Zertifikat anzeigen
- **PDF Report - Custom Tag** → Freitext-Tag auf dem Zertifikat

Alle hier gemachten Änderungen werden persistent in `/etc/wype/wype.conf` gespeichert.

---

## Bugs

* WYPE: [https://github.com/NKr-BKRennen/wype](https://github.com/NKr-BKRennen/wype)
* Original nwipe: [https://github.com/martijnvanbrummelen/nwipe](https://github.com/martijnvanbrummelen/nwipe)

## Lizenz

Wype ist lizenziert unter der **GNU General Public License v2.0**.
Siehe `LICENSE` für Details.

---

## Versionierung

Wype verwendet [Semantic Versioning](https://semver.org/): `MAJOR.MINOR.PATCH`

- **MAJOR**: Inkompatible Änderungen (z.B. neues Config-Format, Breaking Changes)
- **MINOR**: Neue Features, rückwärtskompatibel
- **PATCH**: Bugfixes, kleine Verbesserungen

---

## Changelog

### v1.2.0 (2026-03-17)

**Add:**
- Disk-Metadaten-Editor (`e`-Taste): Hostname + Inventarnummer in einem Dialog mit Tab-Wechsel
- Hilfe-Seite (`h`-Taste) mit allen Tastenbelegungen und Erklärungen
- Changelog-Ansicht (`l`-Taste) direkt in der GUI
- E-Mail-Status-Anzeige im Options-Fenster (Aktiv/Deaktiviert)
- Warnung beim Wipe-Start wenn Hostname/Inventarnummer fehlt (pro Festplatte bestätigen)
- Sammel-E-Mail: alle PDF-Zertifikate in einer E-Mail nach Enter-Bestätigung
- Benachrichtigungs-E-Mail wenn Wipe fertig ("bitte am Gerät bestätigen")
- Lokale PDFs werden nach erfolgreichem E-Mail-Versand automatisch gelöscht
- Post-Wipe E-Mail-Status-Feedback im Log

**Change:**
- Footer überarbeitet: übersichtlichere Tastenbelegung
- Tastenbelegung: `e`=Edit Disk, `h`=Hilfe, `l`=Changelog (statt H/I für einzelne Felder)
- "Inventarnummer" wird in der GUI ausgeschrieben (statt "Inventarnr")

### v1.1.0 (2026-03-17)

**Add:**
- Per-Disk Metadaten: Hostname und Inventarnummer pro Festplatte
- Automatischer E-Mail-Versand der PDF-Zertifikate per SMTP
- Secure Erase / Sanitize Methoden für ATA, NVMe und SCSI (Hardware-basiert)
- ASCII-Art "BK RENNEN" Logo im GUI-Header
- `build.sh` und `update.sh` Skripte für vereinfachtes Bauen und Aktualisieren
- E-Mail-Konfiguration in `/etc/wype/wype.conf`

**Fix:**
- Terminal-Hintergrund bleibt nach Beenden (Ctrl+C) nicht mehr blau
- Footer-Leiste hat jetzt einheitlichen Hintergrund (kein weißer Balken mehr)
- [IN USE] und [HS? YES] Tags sind jetzt rot auf blau statt rot auf weiß
- Hostname/Inventarnummer werden jetzt zuverlässig auf das PDF-Zertifikat geschrieben
- Barcode im PDF-Zertifikat deaktiviert

**Change:**
- Projekt umbenannt von nwipe/BKR_NWIPE zu Wype
- Modernisierte GUI: Farbschema (Teal/Navy/Yellow), farbige Status-Tags, Progress Bars
- Eigenes Versionierungsschema (Semantic Versioning)
- README komplett auf Deutsch, nur Debian 13

**Remove:**
- Unterstützung für andere Distributionen (nur noch Debian 13)
- Barcode auf PDF-Zertifikat (Code bleibt erhalten, nur deaktiviert)

### v1.0.0 (2026-03-16)

**Add:**
- Initiales Release basierend auf nwipe 0.40
- BKR-Branding: Logo im PDF-Zertifikat
- PDF-Lösch-Zertifikate mit Organisations- und Kundendetails
