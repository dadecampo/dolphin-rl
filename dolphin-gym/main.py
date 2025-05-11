import time
from comunication.named_pipe_sender import NamedPipeSender

sender = NamedPipeSender("MyPipe")
while(True):
    sender.connect()    
    if sender.send_command("COMANDO_TEST"):
        print("Comando inviato con successo.")
    else:
        print("Invio fallito.")
    time.sleep(0.2)
        