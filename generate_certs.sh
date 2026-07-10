#!/bin/bash
# ==============================================================================
# Script d'automatisation de génération de certificats TLS pour LITH Server
# Context: Kinshasa, vx.x.x Development
# ==============================================================================

CERT_DIR="certs"
KEY_FILE="$CERT_DIR/server.key"
CRT_FILE="$CERT_DIR/server.crt"

echo "[*] Initialisation de la configuration TLS..."
mkdir -p "$CERT_DIR"

if [ -f "$KEY_FILE" ] && [ -f "$CRT_FILE" ]; then
    echo "[!] Des certificats existants ont été détectés. Regénération en cours..."
fi

# Génération sans invite de commande (Non-interactive) via -subj
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout "$KEY_FILE" \
  -out "$CRT_FILE" \
  -subj "/C=CD/L=Kinshasa/O=FomaDev/CN=localhost" 2>/dev/null

if [ $? -eq 0 ]; then
    echo "[+] Certificats générés avec succès !"
    echo "    -> Clé privée : $KEY_FILE"
    echo "    -> Certificat : $CRT_FILE"
    chmod 600 "$KEY_FILE" "$CRT_FILE"
else
    echo "[-] Erreur critique lors de la génération des certificats avec OpenSSL."
    exit 1
fi