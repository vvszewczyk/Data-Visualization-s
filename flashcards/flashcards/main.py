import random
import io
from docx import Document
from docx.oxml.ns import qn
import tkinter as tk
from tkinter import messagebox
from PIL import Image, ImageTk

# Funkcja do wyciągania obrazka z akapitu, jeśli jest
def extract_image_from_paragraph(paragraph, doc):
    # Iterujemy po wszystkich fragmentach (runs) akapitu
    for run in paragraph.runs:
        # Szukamy elementów rysunkowych – jeśli są, to mamy obrazek
        drawing_elements = run.element.xpath('.//w:drawing')
        if drawing_elements:
            # Szukamy elementu <a:blip>, który przechowuje informację o obrazku
            blip_elements = run.element.xpath('.//a:blip')
            if blip_elements:
                # Pobieramy atrybut 'r:embed', czyli identyfikator obrazu
                rId = blip_elements[0].get(qn('r:embed'))
                # Pobieramy część dokumentu powiązaną z obrazkiem
                image_part = doc.part.related_parts[rId]
                image_bytes = image_part.blob
                try:
                    image = Image.open(io.BytesIO(image_bytes))
                    return image
                except Exception as e:
                    print("Błąd przy otwieraniu obrazka:", e)
    return None

# Funkcja do wczytywania pytań z pliku .docx, zmodyfikowana o sprawdzanie obrazka
def load_questions_from_docx(filename):
    doc = Document(filename)
    questions = []
    current_question = None
    answers = []
    correct_answer = None
    image = None  # Zmienna na obrazek (jeśli wystąpi) dla danego pytania

    # Iterujemy po akapitach w dokumencie
    for para in doc.paragraphs:
        line = para.text.strip()

        # Jeśli akapit jest pusty, ale może zawierać obrazek – sprawdzamy:
        if not line:
            potential_image = extract_image_from_paragraph(para, doc)
            if potential_image is not None and image is None:
                image = potential_image
            continue

        # Sprawdzamy, czy akapit zaczyna się od numeru pytania (np. "1. Treść...")
        if line[0].isdigit() and ". " in line:
            # Jeśli już mamy poprzednie pytanie, zapisujemy je do listy
            if current_question is not None and answers:
                questions.append({
                    "question": current_question,
                    "answers": answers,
                    "correct": correct_answer,
                    "image": image  # Może być None, gdy brak obrazka
                })
            # Nowe pytanie – pobieramy treść pytania
            current_question = line.split(". ", 1)[1].strip()
            answers = []  # Resetujemy odpowiedzi
            correct_answer = None
            image = None  # Resetujemy obrazek dla nowego pytania

        # Jeśli akapit zaczyna się od litery i znaku ")" (odpowiedź, np. "a) Odpowiedź")
        elif line[0].isalpha() and line[1] == ')':
            answer_text = line.split(" ", 1)[1].strip()
            answers.append(answer_text)
            # Jeśli odpowiedź jest pogrubiona – traktujemy ją jako poprawną
            if para.runs and any(run.bold for run in para.runs):
                correct_answer = len(answers) - 1
        else:
            # Dodatkowy akapit – sprawdzamy, czy zawiera obrazek
            if image is None:
                potential_image = extract_image_from_paragraph(para, doc)
                if potential_image is not None:
                    image = potential_image

    # Zapisujemy ostatnie pytanie
    if current_question is not None and answers:
        questions.append({
            "question": current_question,
            "answers": answers,
            "correct": correct_answer,
            "image": image
        })

    return questions

# Funkcja mieszająca pytania i odpowiedzi
def shuffle_questions_and_answers(questions):
    random.shuffle(questions)  # Mieszamy kolejność pytań
    for question in questions:
        zipped = list(zip(question["answers"], range(len(question["answers"]))))
        random.shuffle(zipped)  # Mieszamy odpowiedzi
        question["answers"], shuffled_indexes = zip(*zipped)
        question["correct"] = shuffled_indexes.index(question["correct"])  # Aktualizacja poprawnej odpowiedzi
    return questions

# Klasa interfejsu quizu
class QuizApp:
    def __init__(self, root, questions):
        self.root = root
        self.questions = questions
        self.current_question_index = 0
        self.score = 0
        self.timer_seconds = 0

        # Konfiguracja okna
        self.root.title("Quiz")
        self.root.geometry("800x600")

        # Etykieta z numeracją pytań (np. "Pytanie 1 z 10")
        self.question_count_label = tk.Label(root, text="", font=("Arial", 14))
        self.question_count_label.pack(pady=5)

        # Etykieta z pytaniem
        self.question_label = tk.Label(root, text="", font=("Arial", 16), wraplength=750, justify="center")
        self.question_label.pack(pady=20)

        # Etykieta na obrazek (jeśli występuje)
        self.image_label = tk.Label(root)
        self.image_label.pack(pady=10)

        # Ramka na przyciski odpowiedzi
        self.buttons_frame = tk.Frame(root)
        self.buttons_frame.pack(pady=10)

        self.answer_buttons = []
        for i in range(4):
            btn = tk.Button(self.buttons_frame, text="", font=("Arial", 12), wraplength=700, height=2, anchor="w",
                            command=lambda i=i: self.check_answer(i))
            btn.pack(pady=5, fill=tk.X)
            self.answer_buttons.append(btn)

        self.status_label = tk.Label(root, text="", font=("Arial", 12))
        self.status_label.pack(pady=10)

        self.next_button = tk.Button(root, text="Następne pytanie", font=("Arial", 14), command=self.next_question)
        self.next_button.pack(pady=10)
        self.next_button.config(state=tk.DISABLED)

        # Zegar w prawym dolnym rogu
        self.timer_label = tk.Label(root, text="Czas: 0:00", font=("Arial", 12))
        self.timer_label.pack(anchor="se", padx=10, pady=10)

        self.current_image = None  # Aby zapobiec usunięciu obrazu przez garbage collector
        self.load_question()
        self.start_timer()

    def start_timer(self):
        minutes, seconds = divmod(self.timer_seconds, 60)
        self.timer_label.config(text=f"Czas: {minutes}:{seconds:02d}")
        self.timer_seconds += 1
        self.root.after(1000, self.start_timer)

    def load_question(self):
        question = self.questions[self.current_question_index]
        # Uaktualniamy etykietę z numeracją pytań
        total_questions = len(self.questions)
        self.question_count_label.config(text=f"Pytanie {self.current_question_index + 1} z {total_questions}")

        # Ustawiamy tekst pytania
        self.question_label.config(text=f"{self.current_question_index + 1}. {question['question']}")

        # Jeśli pytanie zawiera obrazek, wyświetlamy go
        if question.get("image") is not None:
            image = question["image"].copy()
            image.thumbnail((400, 300))
            self.current_image = ImageTk.PhotoImage(image)
            self.image_label.config(image=self.current_image)
        else:
            self.image_label.config(image="")

        # Ustawiamy teksty odpowiedzi
        for i, answer in enumerate(question["answers"]):
            self.answer_buttons[i].config(text=f"{['a', 'b', 'c', 'd'][i]}) {answer}", state=tk.NORMAL)

        self.status_label.config(text="")
        self.next_button.config(state=tk.DISABLED)

    def check_answer(self, index):
        question = self.questions[self.current_question_index]
        correct_index = question["correct"]

        if index == correct_index:
            self.status_label.config(text="Poprawna odpowiedź! 😊", fg="green")
            self.score += 1
        else:
            self.status_label.config(
                text=f"Niepoprawna odpowiedź. Poprawna to: {['a', 'b', 'c', 'd'][correct_index]}",
                fg="red"
            )

        for btn in self.answer_buttons:
            btn.config(state=tk.DISABLED)

        self.next_button.config(state=tk.NORMAL)

    def next_question(self):
        self.current_question_index += 1
        if self.current_question_index < len(self.questions):
            self.load_question()
        else:
            messagebox.showinfo("Koniec quizu", f"Twój wynik: {self.score}/{len(self.questions)}")
            self.root.quit()

# Wczytanie pytań i uruchomienie aplikacji
filename = "pytania3.docx"
questions = load_questions_from_docx(filename)
shuffled_questions = shuffle_questions_and_answers(questions)

root = tk.Tk()
app = QuizApp(root, shuffled_questions)
root.mainloop()
