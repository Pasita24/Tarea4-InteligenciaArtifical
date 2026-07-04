/**
 * Wave Function Collapse - Generador de Ciudad Procedural
 * Inspirado en Townscaper (Oskar Stålberg, 2021)
 *
 * Tarea 4 - Inteligencia Artificial para Videojuegos
 * Universidad de Talca - Ingeniería en Desarrollo de Videojuegos y RV
 *
 * Uso: ./wfc_city [ancho] [alto] [semilla] [pixeles_por_celda]
 * Ejemplo: ./wfc_city 30 30 42 20
 */

#include <iostream>
#include <vector>
#include <set>
#include <queue>
#include <random>
#include <fstream>
#include <string>
#include <algorithm>
#include <ctime>

// =====================================================================
// DEFINICIÓN DE TESELAS
// =====================================================================

enum TileType {
    WATER    = 0,  // Agua (borde de ciudad costera)
    GROUND   = 1,  // Suelo vacío / terreno libre
    BUILDING = 2,  // Bloque de edificio
    ROAD     = 3,  // Calle / camino
    PARK     = 4,  // Área verde / plaza
    TILE_COUNT
};

const char* TILE_NAMES[TILE_COUNT] = {
    "Agua", "Suelo", "Edificio", "Calle", "Parque"
};

// =====================================================================
// COLORES PPM (R, G, B)
// =====================================================================

struct Color { unsigned char r, g, b; };

const Color TILE_COLORS[TILE_COUNT] = {
    {100, 149, 237},  // WATER    — azul aciano
    {180, 200, 130},  // GROUND   — verde musgo claro
    {210, 155,  90},  // BUILDING — beige cálido / naranja edificio
    {150, 150, 150},  // ROAD     — gris asfalto
    { 90, 170,  90},  // PARK     — verde brillante
};

// =====================================================================
// REGLAS DE ADYACENCIA
// ADJACENCY[A][B] = true  =>  la tesela A puede estar junto a la tesela B
// Las reglas son simétricas (se cumple en ambas direcciones).
// =====================================================================

const bool ADJACENCY[TILE_COUNT][TILE_COUNT] = {
    //          WATER   GROUND  BUILDING  ROAD    PARK
    /* WATER */  { true,  true,  false,   false,  false },
    /* GROUND */ { true,  true,  true,    true,   true  },
    /* BUILD  */ { false, true,  true,    true,   false },
    /* ROAD   */ { false, true,  true,    true,   true  },
    /* PARK   */ { false, true,  false,   true,   true  },
};

// =====================================================================
// CLASE WFC
// =====================================================================

class WFC {
public:
    int width, height;
    // grid[y][x] = conjunto de teselas posibles para esa celda
    std::vector<std::vector<std::set<int>>> grid;
    std::mt19937 rng;
    bool contradiction;

    WFC(int w, int h, unsigned int seed)
        : width(w), height(h), rng(seed), contradiction(false)
    {
        // Inicialización: todas las celdas comienzan en superposición total
        grid.assign(height, std::vector<std::set<int>>(width));
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                for (int t = 0; t < TILE_COUNT; t++)
                    grid[y][x].insert(t);
    }

    // Retorna la celda con menor entropía (más restringida, > 1 posibilidad).
    // Retorna {-1,-1} si todas están colapsadas (éxito) o hay contradicción.
    std::pair<int,int> getMinEntropyCell() {
        int minEntropy = TILE_COUNT + 1;
        std::pair<int,int> result = {-1, -1};

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int sz = (int)grid[y][x].size();
                if (sz == 0) {
                    contradiction = true;
                    return {-1, -1};
                }
                if (sz > 1 && sz < minEntropy) {
                    minEntropy = sz;
                    result = {x, y};
                }
            }
        }
        return result;
    }

    // Colapsa la celda (x, y) eligiendo aleatoriamente una de sus teselas posibles.
    void collapse(int x, int y) {
        auto& possible = grid[y][x];
        if (possible.empty()) { contradiction = true; return; }

        int idx = std::uniform_int_distribution<int>(0, (int)possible.size() - 1)(rng);
        auto it = possible.begin();
        std::advance(it, idx);
        int chosen = *it;

        possible.clear();
        possible.insert(chosen);
    }

    // Propagación de restricciones via BFS desde la celda colapsada.
    // Elimina de las celdas vecinas las teselas que ya no son compatibles.
    void propagate(int startX, int startY) {
        const int dx[] = {0, 0, 1, -1};
        const int dy[] = {1, -1, 0, 0};

        std::queue<std::pair<int,int>> q;
        q.push({startX, startY});

        while (!q.empty()) {
            auto [cx, cy] = q.front();
            q.pop();

            for (int d = 0; d < 4; d++) {
                int nx = cx + dx[d];
                int ny = cy + dy[d];
                if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;

                auto& neighborSet = grid[ny][nx];
                if (neighborSet.size() == 1) continue; // ya colapsada

                // Construir conjunto de teselas vecinas aún válidas
                std::set<int> valid;
                for (int nt : neighborSet) {
                    for (int ct : grid[cy][cx]) {
                        if (ADJACENCY[ct][nt]) {
                            valid.insert(nt);
                            break;
                        }
                    }
                }

                // Si se redujeron las posibilidades, encolar para propagar
                if (valid.size() < neighborSet.size()) {
                    neighborSet = valid;
                    if (neighborSet.empty()) {
                        contradiction = true;
                        return;
                    }
                    q.push({nx, ny});
                }
            }
        }
    }

    // Ejecuta el algoritmo WFC completo.
    // Retorna true si se completó sin contradicciones.
    bool run() {
        while (true) {
            auto [x, y] = getMinEntropyCell();
            if (contradiction) return false;
            if (x == -1) return true; // todas las celdas colapsadas

            collapse(x, y);
            if (contradiction) return false;
            propagate(x, y);
            if (contradiction) return false;
        }
    }

    // Obtiene la tesela final de una celda colapsada.
    int getTile(int x, int y) const {
        if (grid[y][x].empty()) return -1;
        return *grid[y][x].begin();
    }

    // Exporta el mapa generado como imagen PPM binaria (P6).
    // cellSize: cantidad de píxeles por celda (recomendado: 16-24).
    void savePPM(const std::string& filename, int cellSize = 20) const {
        int imgW = width  * cellSize;
        int imgH = height * cellSize;

        std::ofstream f(filename, std::ios::binary);
        if (!f.is_open()) {
            std::cerr << "Error: no se pudo abrir el archivo " << filename << "\n";
            return;
        }

        // Cabecera PPM
        f << "P6\n" << imgW << " " << imgH << "\n255\n";

        for (int py = 0; py < imgH; py++) {
            for (int px = 0; px < imgW; px++) {
                int cx = px / cellSize;
                int cy = py / cellSize;
                int tile = getTile(cx, cy);

                Color c = (tile >= 0) ? TILE_COLORS[tile] : Color{255, 0, 255};

                // Línea de grilla sutil para separar celdas visualmente
                bool isBorder = (cellSize > 5) &&
                                (px % cellSize == 0 || py % cellSize == 0);
                if (isBorder) {
                    c.r = (unsigned char)(c.r * 0.65);
                    c.g = (unsigned char)(c.g * 0.65);
                    c.b = (unsigned char)(c.b * 0.65);
                }

                f.put(c.r);
                f.put(c.g);
                f.put(c.b);
            }
        }

        f.close();
        std::cout << "  Imagen guardada: " << filename
                  << " (" << imgW << "x" << imgH << " px)\n";
    }

    // Imprime estadísticas del mapa generado.
    void printStats(unsigned int seed) const {
        int counts[TILE_COUNT] = {};
        int total = width * height;
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                counts[getTile(x, y)]++;

        std::cout << "  Semilla  : " << seed << "\n";
        std::cout << "  Grilla   : " << width << "x" << height
                  << " (" << total << " celdas)\n";
        std::cout << "  Distribucion de teselas:\n";
        for (int t = 0; t < TILE_COUNT; t++) {
            std::cout << "    " << TILE_NAMES[t] << ": "
                      << counts[t] << " celdas ("
                      << (100 * counts[t]) / total << "%)\n";
        }
    }
};

// =====================================================================
// MAIN
// =====================================================================

int main(int argc, char* argv[]) {
    // Parámetros configurables (pueden pasarse por línea de comandos)
    int width    = (argc > 1) ? std::stoi(argv[1]) : 30;
    int height   = (argc > 2) ? std::stoi(argv[2]) : 30;
    unsigned int seed = (argc > 3)
                        ? (unsigned int)std::stoul(argv[3])
                        : (unsigned int)std::time(nullptr);
    int cellSize = (argc > 4) ? std::stoi(argv[4]) : 20;

    const int MAX_RETRIES = 30;

    std::cout << "=== Wave Function Collapse - Generador de Ciudad ===\n";
    std::cout << "Parametros: " << width << "x" << height
              << " | semilla base=" << seed
              << " | celda=" << cellSize << "px\n\n";

    for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
        unsigned int currentSeed = seed + (unsigned int)attempt;
        WFC wfc(width, height, currentSeed);
        bool success = wfc.run();

        if (success) {
            std::cout << "Generacion exitosa (intento " << (attempt + 1) << ")\n";
            wfc.printStats(currentSeed);

            std::string fname = "output_" + std::to_string(width)
                              + "x" + std::to_string(height)
                              + "_seed" + std::to_string(currentSeed)
                              + ".ppm";
            wfc.savePPM(fname, cellSize);
            std::cout << "\nListo. Para visualizar: abre el archivo .ppm\n";
            std::cout << "con GIMP, IrfanView, o convierte con: convert "
                      << fname << " output.png\n";
            return 0;
        } else {
            std::cout << "Contradiccion en intento " << (attempt + 1)
                      << " (semilla " << currentSeed << "), reintentando...\n";
        }
    }

    std::cerr << "\nError: no se genero un mapa valido en "
              << MAX_RETRIES << " intentos.\n";
    return 1;
}
