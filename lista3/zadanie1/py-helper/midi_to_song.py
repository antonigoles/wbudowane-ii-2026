import mido

def midi_to_c_song(midi_path):
    mid = mido.MidiFile(midi_path)
    
    # Mapowanie nut MIDI (skala chromatyczna mod 12) na litery C, D, E, F, G, A, B
    pitch_to_char = {
        0: 'C', 1: 'C', 
        2: 'D', 3: 'D', 
        4: 'E', 
        5: 'F', 6: 'F', 
        7: 'G', 8: 'G', 
        9: 'A', 10: 'A', 
        11: 'B'
    }

    ticks_per_beat = mid.ticks_per_beat  # Zazwyczaj 480 lub 960 tików na ćwierćnutę
    
    # Zbieramy wszystkie komunikaty note_on / note_off ze wszystkich ścieżek z czasem absolutnym
    events = []
    for track in mid.tracks:
        abs_time = 0
        for msg in track:
            abs_time += msg.time
            if msg.type in ('note_on', 'note_off'):
                events.append((abs_time, msg))

    # Sortujemy zdarzenia chronologicznie
    events.sort(key=lambda x: x[0])

    if not events:
        print("// Brak nut w pliku MIDI!")
        return

    # Standardowe dzielniki w relacji do ćwierćnuty (ticks_per_beat)
    # 1 = cała nuta (4 * beat), 2 = półnuta (2 * beat), 4 = ćwierćnuta (1 * beat), 8 = ósemka (0.5 * beat)
    standard_divisors = [
        (1, ticks_per_beat * 4),
        (2, ticks_per_beat * 2),
        (4, ticks_per_beat),
        (8, ticks_per_beat / 2),
        (16, ticks_per_beat / 4)
    ]

    def ticks_to_divisor(ticks):
        # Znajduje najbliższy standardowy dzielnik dla danej liczby tików
        closest = min(standard_divisors, key=lambda x: abs(x[1] - ticks))
        return closest[0]

    # Przetwarzanie komunikatów na nuty i pauzy
    song_entries = []
    current_abs_time = 0
    
    # Uproszczone parsowanie pierwszej ścieżki melodycznej
    # (Zakładamy plik MIDI z jedną główną linią melodyczną)
    
    last_event_end = 0
    
    print("// Wygenerowano automatycznie z pliku MIDI")
    print("static const uint8_t SONG[][2] = {")

    # Przechodzimy przez unikalne momenty w czasie
    # Dla uproszczenia skryptu wyciągamy sekwencję note_on
    i = 0
    while i < len(events):
        time_abs, msg = events[i]
        
        if msg.type == 'note_on' and msg.velocity > 0:
            note_start = time_abs
            
            # Sprawdź czy była przerwa (pauza 'H') od końca poprzedniej nuty
            if note_start > last_event_end:
                pause_ticks = note_start - last_event_end
                if pause_ticks > (ticks_per_beat / 8): # Ignorujemy mikroskopijne przerwy techniczne
                    pause_div = ticks_to_divisor(pause_ticks)
                    print(f"    {{'H', {pause_div}}},")

            note_char = pitch_to_char.get(msg.note % 12, 'C')
            
            # Szukamy odpowiadającego note_off dla tej nuty, aby ustalić jej długość
            note_duration_ticks = ticks_per_beat # domyślnie ćwierćnuta
            for j in range(i + 1, len(events)):
                t_end, m_end = events[j]
                if m_end.type in ('note_off', 'note_on') and m_end.note == msg.note:
                    note_duration_ticks = max(10, t_end - note_start)
                    break
            
            divisor = ticks_to_divisor(note_duration_ticks)
            print(f"    {{'{note_char}', {divisor}}},")
            
            last_event_end = note_start + note_duration_ticks
        i += 1

    print("};")

if __name__ == "__main__":
    # Podaj nazwę swojego pliku MIDI
    midi_file = "mario.mid"
    midi_to_c_song(midi_file)