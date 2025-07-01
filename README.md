# Renderowanie Terenu z Symulacją Samolotu

## Opis projektu
Ten projekt to bardzo prosty symulator samolotu z renderowanym terenem 3D jako projekt na przedmiot Grafika komputerowa i wizualizacja PUT semestr 4. Generowanie terenu zostało zainspirowane poradnikami z kanału YouTube OGLDEV, które dostarczyły cennych informacji o technikach renderowania terenu.

##Link do filmu
[![Zobacz demo na YouTube](https://img.youtube.com/vi/YOUR_VIDEO_ID/0.jpg)](https://www.youtube.com/watch?v=3uT_l1lke9I)

## Użyte technologie
- OpenGL 3.3+
- GLFW – obsługa okna
- GLEW – ładowanie rozszerzeń OpenGL
- ImGui – interfejs użytkownika
- C++ – główna logika programu

## Funkcje
- Proceduralnie generowany teren za pomocą algorytmu przesunięcia środka (midpoint displacement)

- Interaktywna symulacja lotu samolotu z realistycznym sterowaniem

- Dynamiczny system kamery z wieloma trybami widoku

- Symulacja stada ptaków (flocking)

- Oświetlenie i cieniowanie w czasie rzeczywistym

- Możliwość dostosowania parametrów terenu

- Interaktywne GUI do regulacji parametrów podczas działania programu

## Sterowanie

### Tryby kamery
-  `F` przełączenie na tryb swobodnej kamery
-  `T`  tryb podążania za samolotem

### Sterowanie samolotem (w trybie podążania kamery)
- `W` - do przodu
- `S` - do tyłu
- `A` - w lewo
- `D` - w prawo
- `+` - w górę
- `-` - w dół 
- `V/B` - obrót w lewo/prawo
- `N/M` - pochylenie w górę/dół

### Ogólne sterowanie
- `ESC` lub `Q` - wyjście z aplikacji
- `R` - resetowanie pozycji kamery
- `C` - wypisanie pozycji kamery
- `E` - przełączenie trybu siatki (wireframe)
- `P` - pauza symulacji
- `SPACE` - przełączenie GUI
- `0` - przełączenie wyświetlania punktów
- `1/2/3` - zmiana trybu śledzenia kostki

## Opcje personalizacji

### Terrain
- Dostosuj rozmiar terenu w pliku `terrain_demo1.cpp`:
  - Zmień wartość  `m_terrainSize`
  - Zmień `m_roughness` , aby uzyskać różne wzory terenu
  -Dostosuj  `m_minHeight` i `m_maxHeight` , by zmienić zakres wysokości

### Ptaki
- Liczba ptaków: funkcja `InitBirds()` 
- Prędkość i zachowanie: klasa `Bird`
- Wygląd ptaków: funkcja `RenderBirdPart` 

### Oświetlenie
- Intensywność i kolor: shader `terrain.fs` 
- Customize ambient and diffuse lighting parameters

### Airplane
- Rozmiar samolotu: zmień `cubeSize` w `InitPlayerCube()`
- Prędkość: zmień `m_cubeSpeed`
- Dystans i wysokość kamery za samolotem: `SetCameraBehindCubeFixed()`

## Informacje dla deweloperów
- Projekt wykorzystuje nowoczesne podejście do OpenGL z użyciem shaderów
- Generowanie terenu opiera się na algorytmie przesunięcia środka, zapewniającym naturalny wygląd krajobrazu
- Symulacja samolotu obejmuje realistyczną fizykę i detekcję kolizji
- System stada ptaków wykorzystuje podstawowe zasady zachowania grupowego

## Wymagania
-Kompilator C++ z obsługą C++11
-OpenGL 3.3 lub wyższy
-GLFW 3.x
-GLEW
-ImGui

## Budowanie projektu
1. Upewnij się, że wszystkie zależności są zainstalowane
2. Skompiluj projekt za pomocą pliku .sln w VS2022

## Credits
- Terrain generation techniques inspired by OGLDEV YouTube channel
- OpenGL and GLFW for graphics and window management
- ImGui for the user interface system 
