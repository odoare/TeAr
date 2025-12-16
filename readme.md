# TeAr - Polyrhythmic MIDI Arpeggiator

![](doc/TeAR.png)

TeAr is an advanced polyrhythmic and polyphonic MIDI arpeggiator plugin. It features four independent arpeggiator engines, each with its own pattern, subdivision, and MIDI output channel. This allows for the creation of complex, evolving musical phrases and textures.

At its core, TeAr is scale-aware. You can select a root note and one of many scale types, and the arpeggiators will intelligently conform to that musical context. A "Follow MIDI In" mode allows the scale's root to be changed dynamically by the notes you play.

## Key Features

*   **Four Independent Arpeggiator Engines**: Create complex polyrhythms and layered melodic lines.
*   **Per-Arp Controls**: Each engine has its own On/Off switch, pattern editor, subdivision, and MIDI output channel.
*   **Powerful Pattern Language**: A rich text-based language for defining arpeggio sequences with modifiers for velocity, octave, and pitch.
*   **Comprehensive Scale Library**: A wide selection of musical scales and modes to define the harmonic landscape.
*   **Visual Feedback**: A real-time scale display highlights the active scale, its root, and the notes currently being played by each arpeggiator, color-coded for clarity.

## Pattern Language Documentation

The pattern string consists of characters that define the arpeggio's behavior at each step.

### Note Commands

| Command | Description |
| :--- | :--- |
| `0` to `6` | Plays a specific degree of the chord/scale (0=fundamental, 1=third, ..., 6=thirteenth). |
| `_` | Sustains the previously played note. |
| `.` | A rest; no note is played. |
| `+` | Plays the next degree in the chord (e.g., from 1 to 2). |
| `-` | Plays the previous degree in the chord (e.g., from 2 to 1). |
| `?` | Plays a random, valid note from the current chord. |
| `"` or `=` | Repeats the last played degree. |

### Pitch Modifiers

These are prefixed to a note command to alter its pitch for that step only.

| Command | Description | Example |
| :--- | :--- | :--- |
| `#` | Pitches the next note up by one semitone. | `#0` |
| `b` | Pitches the next note down by one semitone. | `b0` |

### Velocity Modifiers

Velocity is specified with a level from 1-8, which maps to a MIDI velocity from 16-127.

| Command | Description | Example |
| :--- | :--- | :--- |
| `vN` | **Local**: Sets velocity for the next note only. | `v80` (plays root at max velocity) |
| `VN` | **Global**: Sets velocity for all subsequent notes until the next `V` command. | `V40` (sets global velocity to 64) |

### Octave Modifiers

| Command | Description | Example |
| :--- | :--- | :--- |
| `oN` | **Local**: Sets octave for the next note only (N is 0-7). | `o30` (plays root in octave 3) |
| `o+` | **Local**: Increases octave by one for the next note only. | `o+0` |
| `o-` | **Local**: Decreases octave by one for the next note only. | `o-0` |
| `ON` | **Global**: Sets octave for all subsequent notes (N is 0-7). | `O5` |
| `O+` | **Global**: Increases the global octave by one. | `O+` |
| `O-` | **Global**: Decreases the global octave by one. | `O-` |

---

## Implemented Scale Modes

| Scale/Mode Name | Intervals (from root) |
| :--- | :--- |
| **Major Scale Modes** | |
| Major (Ionian) | 0, 2, 4, 5, 7, 9, 11 |
| Dorian | 0, 2, 3, 5, 7, 9, 10 |
| Phrygian | 0, 1, 3, 5, 7, 8, 10 |
| Lydian | 0, 2, 4, 6, 7, 9, 11 |
| Mixolydian | 0, 2, 4, 5, 7, 9, 10 |
| Aeolian | 0, 2, 3, 5, 7, 8, 10 |
| Locrian | 0, 1, 3, 5, 6, 8, 10 |
| **Melodic Minor Modes** | |
| Melodic Minor | 0, 2, 3, 5, 7, 9, 11 |
| Dorian b9 | 0, 1, 3, 5, 7, 9, 10 |
| Lydian #5 | 0, 2, 4, 6, 8, 9, 11 |
| Lydian b7 (Bartok) | 0, 2, 4, 6, 7, 9, 10 |
| Mixolydian b13 | 0, 2, 4, 5, 7, 8, 10 |
| Locrian Natural 9 | 0, 2, 3, 5, 6, 8, 10 |
| Altered | 0, 1, 3, 4, 6, 8, 10 |
| **Harmonic Minor Modes** | |
| Harmonic Minor | 0, 2, 3, 5, 7, 8, 11 |
| Locrian Natural 6 | 0, 1, 3, 5, 6, 9, 10 |
| Ionian #5 | 0, 2, 4, 5, 8, 9, 11 |
| Dorian #4 | 0, 2, 3, 6, 7, 9, 10 |
| Phrygian Dominant | 0, 1, 4, 5, 7, 8, 10 |
| Lydian #2 | 0, 3, 4, 6, 7, 9, 11 |
| Altered bb7 (Ultralocrian) | 0, 1, 3, 4, 6, 8, 9 |
| **Other 7-note scales** | |
| Harmonic Major | 0, 2, 4, 5, 7, 8, 11 |
| Double Harmonic Major | 0, 1, 4, 5, 7, 8, 11 |
| Hungarian Minor | 0, 2, 3, 6, 7, 8, 11 |
| Neapolitan Major | 0, 1, 4, 5, 7, 9, 11 |
| Neapolitan Minor | 0, 1, 3, 5, 7, 8, 11 |
| **Non-diatonic scales** | |
| Major Pentatonic | 0, 2, 4, 7, 9 |
| Minor Pentatonic | 0, 3, 5, 7, 10 |
| Blues | 0, 3, 5, 6, 7, 10 |
| Whole Tone | 0, 2, 4, 6, 8, 10 |
| Octatonic (Half-Whole) | 0, 1, 3, 4, 6, 7, 9, 10 |
| Octatonic (Whole-Half) | 0, 2, 3, 5, 6, 8, 9, 11 |
