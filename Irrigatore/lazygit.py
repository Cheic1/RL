import os

VERSION_FILE = "src/main.cpp"
VERSION_DEFINE = "APP_VERSION"

def increment_version():
    # Leggi la versione attuale dal file
    if os.path.exists(VERSION_FILE):
        with open(VERSION_FILE, 'r') as file:
            content = file.read()
            start = content.find(VERSION_DEFINE + ' "') + len(VERSION_DEFINE + ' "')
            end = content.find('"', start)
            current_version = content[start:end]
    else:
        current_version = "0.0.0"

    # Dividi la versione in parti
    version_parts = current_version.split('.')

    # Incrementa l'ultima parte della versione
    version_parts[-1] = str(int(version_parts[-1]) + 1)

    # Ricomponi la nuova versione
    new_version = '.'.join(version_parts)

    # Prepara il contenuto del nuovo file
    new_content = f'#define {VERSION_DEFINE} "{new_version}"\n'

    # Scrivi la nuova versione nel file
    with open(VERSION_FILE, 'w') as file:
        file.write(new_content)

    print(f"Versione aggiornata da {current_version} a {new_version}")

if __name__ == "__main__":
    increment_version()