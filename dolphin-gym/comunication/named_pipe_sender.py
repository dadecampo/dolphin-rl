import time
import win32file
import pywintypes


class NamedPipeSender:
    def __init__(self, pipe_name: str, retry_interval: float = 1.0, max_retries: int = 5):
        self.pipe_path = fr'\\.\pipe\{pipe_name}'
        self.retry_interval = retry_interval
        self.max_retries = max_retries
        self.handle = None

    def connect(self) -> bool:
        """Tenta di connettersi alla pipe, con retries."""
        retries = 0
        while retries < self.max_retries:
            try:
                self.handle = win32file.CreateFile(
                    self.pipe_path,
                    win32file.GENERIC_WRITE,
                    0,
                    None,
                    win32file.OPEN_EXISTING,
                    0,
                    None
                )
                return True
            except pywintypes.error as e:
                if e.args[0] == 2:
                    print("Pipe non trovata, riprovo...")
                elif e.args[0] == 231:  # All pipe instances are busy
                    print("Tutte le istanze della pipe sono occupate, riprovo...")
                else:
                    print(f"Errore durante la connessione: {e}")
                    break
                retries += 1
                time.sleep(self.retry_interval)
        return False

    def send_command(self, command: str) -> bool:
        """Invia un comando alla pipe, gestendo eventuali errori."""
        if not self.handle:
            return False

        try:
            if not command.endswith('\n'):
                command += '\n'
            win32file.WriteFile(self.handle, command.encode('utf-8'))
            return True
        except pywintypes.error as e:
            if e.args[0] == 109:
                print("Pipe interrotta (broken pipe).")
            else:
                print(f"Errore durante l'invio del comando: {e}")
            return False

    def close(self):
        if self.handle:
            try:
                self.handle.Close()
            except Exception:
                pass
            self.handle = None

    def __del__(self):
        self.close()
