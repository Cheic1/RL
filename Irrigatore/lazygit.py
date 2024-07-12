import re
import subprocess
import sys
import shutil
import os


VERSION_FILE = "src/main.cpp"
VERSION_DEFINE = "APP_VERSION"

def increment_version():
    # ... [Il codice precedente per incrementare la versione rimane invariato] ...
    # Leggi il contenuto del file
    try:
        with open(VERSION_FILE, 'r') as file:
            content = file.read()
    except FileNotFoundError:
        print(f"File {VERSION_FILE} non trovato. Creazione di un nuovo file.")
        content = f'#define {VERSION_DEFINE} "0.0.0"\n'

    # Trova la riga con APP_VERSION
    pattern = rf'#define\s+{VERSION_DEFINE}\s+"(\d+\.\d+\.\d+)"'
    match = re.search(pattern, content)

    if match:
        current_version = match.group(1)
    else:
        print(f"APP_VERSION non trovato. Inizializzazione a 0.0.0")
        current_version = "0.0.0"

    # Dividi la versione in parti
    version_parts = current_version.split('.')

    # Incrementa l'ultima parte della versione
    version_parts[-1] = str(int(version_parts[-1]) + 1)

    # Ricomponi la nuova versione
    new_version = '.'.join(version_parts)

    # Sostituisci la vecchia versione con la nuova
    new_content = re.sub(pattern, f'#define {VERSION_DEFINE} "{new_version}"', content)

    # Scrivi il contenuto aggiornato nel file
    with open(VERSION_FILE, 'w') as file:
        file.write(new_content)

    print(f"Versione aggiornata da {current_version} a {new_version}")
    print(f"Versione aggiornata da {current_version} a {new_version}")
    return new_version

def run_command(command):
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    stdout, stderr = process.communicate()
    if process.returncode != 0:
        print(f"Errore nell'esecuzione del comando: {command}")
        print(f"Errore: {stderr.decode()}")
        sys.exit(1)
    return stdout.decode()

def main():
    if not os.path.exists('FW'):
        os.makedirs('FW')

    new_version = increment_version()

    # Esegui pio run
    print("Esecuzione di 'pio run --environment d1'...")
    run_command("pio run --environment d1")

    # Esegui git add
    print("Aggiunta dei file modificati a git...")
    run_command("git add .")



    # Cerca i file .bin, rinominali e copiali nella cartella /FW
    print("Cercando e copiando i file .bin...")
    for root, dirs, files in os.walk('.'):
        if 'FW' in root:
            continue
        for file in files:
            if file.endswith('.bin'):
                old_path = os.path.join(root, file)
                new_filename = f"{os.path.splitext(file)[0]}_{new_version}.bin"
                new_path = os.path.join('FW', new_filename)
                shutil.copy2(old_path, new_path)
                print(f"Copiato {old_path} in {new_path}")
    
    # Trova l'ultimo file .bin copiato
    last_bin_file = max(
        [f for f in os.listdir('FW') if f.endswith('.bin')],
        key=lambda x: os.path.getmtime(os.path.join('FW', x))
    )
    
    
    # Esegui git commit
    commit_message = sys.argv[1] if len(sys.argv) > 1 else f"Aggiornamento versione a {new_version}"
    print(f"Commit dei cambiamenti: {commit_message}")
    run_command(f'git commit -a -m "{new_version} - {commit_message}"')
    
    # Esegui git push
    print("Push dei cambiamenti al repository remoto...")
    run_command("git push")
    
    print("Operazioni completate con successo.")
    print(f"Ultimo file .bin copiato: {last_bin_file}")

if __name__ == "__main__":
    main()