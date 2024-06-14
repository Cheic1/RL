import tkinter as tk
from tkinter import filedialog
from pydrive.auth import GoogleAuth
from pydrive.drive import GoogleDrive

# Funzione per gestire il clic sul pulsante "Scarica"
def scarica_file():
    # Autenticazione con Google Drive
    gauth = GoogleAuth()
    gauth.LocalWebserverAuth()

    # Inizializza l'API di Google Drive
    drive = GoogleDrive(gauth)

    # Apre la finestra di dialogo per selezionare il file da scaricare
    file_path = filedialog.askopenfilename(title="Seleziona il file da scaricare")

    # Estrai l'ID del file dal percorso selezionato
    file_id = estrai_id_da_percorso(file_path)

    # Scarica il file
    file = drive.CreateFile({'id': file_id})
    file.GetContentFile(file_path)  # Salva il file nella posizione selezionata dall'utente

# Funzione per estrarre l'ID del file dal percorso selezionato
def estrai_id_da_percorso(file_path):
    # Esempio di estrazione dell'ID dal percorso del file
    # Implementa la tua logica per estrarre l'ID dal percorso del file
    # In questo esempio, assumiamo che l'ID sia l'ultima parte del percorso
    parts = file_path.split('/')
    return parts[-1]

# Creazione della finestra principale dell'applicazione
root = tk.Tk()
root.title("Seleziona e Scarica File da Google Drive")

# Aggiungi un pulsante per avviare il processo di download
btn_scarica = tk.Button(root, text="Seleziona e Scarica File", command=scarica_file)
btn_scarica.pack(pady=20)

# Avvia il loop principale dell'applicazione
root.mainloop()
