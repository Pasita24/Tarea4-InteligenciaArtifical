/**
 * Wave Function Collapse - Generador de Ciudad Procedural
 * Inspirado en Townscaper (Oskar Stålberg, 2021)
 *
 * Tarea 4 - Inteligencia Artificial para Videojuegos
 * Universidad de Talca - Ingeniería en Desarrollo de Videojuegos y RV
 *
 * Uso: ./wfc_city [ancho] [alto] [semilla] [pixeles_por_celda] [ruleset]
 * ruleset: 0=original, 1=parque con lago, 2=estricta
 * Ejemplo: ./wfc_city 30 30 42 20 1
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
#include <cstring>   // para memcpy

// =====================================================================
// DEFINICIÓN DE TESELAS
// =====================================================================

enum TileType {
    WATER    = 0,
    GROUND   = 1,
    BUILDING = 2,
    ROAD     = 3,
    PARK     = 4,
    TILE_COUNT
};

const char* TILE_NAMES[TILE_COUNT] = {
    "Agua", "Suelo", "Edificio", "Calle", "Parque"
};

// =====================================================================
// COLORES (R, G, B) - se usan igual para PPM
// =====================================================================

struct Color { unsigned char r, g, b; };

const Color TILE_COLORS[TILE_COUNT] = {
    {100, 149, 237},  // WATER    — azul aciano
    {180, 200, 130},  // GROUND   — verde musgo claro
    {210, 155,  90},  // BUILDING — beige cálido
    {150, 150, 150},  // ROAD     — gris asfalto
    { 90, 170,  90},  // PARK     — verde brillante
};

// =====================================================================
// CLASE WFC (con reglas dinámicas y pesos opcionales)
// =====================================================================

class WFC {
public:
    int width, height;
    std::vector<std::vector<std::set<int>>> grid;
    std::mt19937 rng;
    bool contradiction;

    // Matriz de adyacencia (dinámica, no global)
    bool adjacency[TILE_COUNT][TILE_COUNT];

    // Pesos para la selección ponderada (todos 1 por defecto)
    double weights[TILE_COUNT];

    // Constructor con ruleset
    WFC(int w, int h, unsigned int seed, int ruleset = 0)
        : width(w), height(h), rng(seed), contradiction(false)
    {
        // Inicializar pesos por defecto (todos 1)
        for (int i = 0; i < TILE_COUNT; i++) weights[i] = 1.0;

        // Cargar las reglas según el ruleset
        loadRules(ruleset);

        // Inicializar grid con todas las teselas posibles
        grid.assign(height, std::vector<std::set<int>>(width));
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                for (int t = 0; t < TILE_COUNT; t++)
                    grid[y][x].insert(t);
    }

    // Carga las reglas de adyacencia según el conjunto elegido
    void loadRules(int ruleset) {
        // Primero, poner todas las reglas a false por defecto
        for (int i = 0; i < TILE_COUNT; i++)
            for (int j = 0; j < TILE_COUNT; j++)
                adjacency[i][j] = false;

        // Definir los conjuntos según ruleset
        if (ruleset == 0) {
            // ========== ORIGINAL (ciudad costera) ==========
            // Agua: solo con agua y suelo
            adjacency[WATER][WATER] = true;
            adjacency[WATER][GROUND] = true;
            adjacency[GROUND][WATER] = true;
            // Suelo: con todo
            for (int t = 0; t < TILE_COUNT; t++) {
                adjacency[GROUND][t] = true;
                adjacency[t][GROUND] = true;
            }
            // Edificio: con suelo, edificio, calle
            adjacency[BUILDING][GROUND] = true;
            adjacency[GROUND][BUILDING] = true;
            adjacency[BUILDING][BUILDING] = true;
            adjacency[BUILDING][ROAD] = true;
            adjacency[ROAD][BUILDING] = true;
            // Calle: con suelo, edificio, calle, parque
            adjacency[ROAD][GROUND] = true;
            adjacency[GROUND][ROAD] = true;
            adjacency[ROAD][ROAD] = true;
            adjacency[ROAD][PARK] = true;
            adjacency[PARK][ROAD] = true;
            // Parque: con suelo, calle, parque
            adjacency[PARK][GROUND] = true;
            adjacency[GROUND][PARK] = true;
            adjacency[PARK][PARK] = true;
            // (ya se agregó ROAD-PARK arriba)
        }
        else if (ruleset == 1) {
            // ========== PARQUE CON LAGO ==========
            // Agua solo con agua y suelo (para formar lagos rodeados de pasto)
            adjacency[WATER][WATER] = true;
            adjacency[WATER][GROUND] = true;
            adjacency[GROUND][WATER] = true;
            // Suelo con todo (es el elemento principal)
            for (int t = 0; t < TILE_COUNT; t++) {
                adjacency[GROUND][t] = true;
                adjacency[t][GROUND] = true;
            }
            // Edificio: solo con suelo y edificio (sin calles ni parques, para no aparecer)
            adjacency[BUILDING][GROUND] = true;
            adjacency[GROUND][BUILDING] = true;
            adjacency[BUILDING][BUILDING] = true;
            // Calle: solo con suelo y calle (para que aparezcan caminos)
            adjacency[ROAD][GROUND] = true;
            adjacency[GROUND][ROAD] = true;
            adjacency[ROAD][ROAD] = true;
            // Parque: solo con suelo y parque (para que crezcan zonas verdes)
            adjacency[PARK][GROUND] = true;
            adjacency[GROUND][PARK] = true;
            adjacency[PARK][PARK] = true;
            // No permitimos edificio con agua, ni calle con agua, ni parque con agua (excepto suelo)
            // Ya está implícito porque solo WATER-GROUND y WATER-WATER son true.
            // Para dar más protagonismo al agua y al parque, podemos ajustar pesos después.
            // Aquí solo definimos reglas.
        }
        else if (ruleset == 2) {
            // ========== ESTRICTA (urbana con manzanas) ==========
            // Agua solo con agua y suelo
            adjacency[WATER][WATER] = true;
            adjacency[WATER][GROUND] = true;
            adjacency[GROUND][WATER] = true;
            // Suelo con todo (sigue siendo conector)
            for (int t = 0; t < TILE_COUNT; t++) {
                adjacency[GROUND][t] = true;
                adjacency[t][GROUND] = true;
            }
            // Edificio solo con suelo y calle (no edificio-edificio, para forzar manzanas)
            adjacency[BUILDING][GROUND] = true;
            adjacency[GROUND][BUILDING] = true;
            adjacency[BUILDING][ROAD] = true;
            adjacency[ROAD][BUILDING] = true;
            // Calle con suelo, edificio, calle, parque
            adjacency[ROAD][GROUND] = true;
            adjacency[GROUND][ROAD] = true;
            adjacency[ROAD][ROAD] = true;
            adjacency[ROAD][PARK] = true;
            adjacency[PARK][ROAD] = true;
            // Parque con suelo, calle, parque
            adjacency[PARK][GROUND] = true;
            adjacency[GROUND][PARK] = true;
            adjacency[PARK][PARK] = true;
        }
        else {
            // Por defecto, cargar el original
            loadRules(0);
        }
    }

    // Método para establecer pesos (se puede llamar desde main)
    void setWeights(double w[TILE_COUNT]) {
        for (int i = 0; i < TILE_COUNT; i++) weights[i] = w[i];
    }

    // Retorna la celda con menor entropía (>1 posibilidad)
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

    // Colapso con selección ponderada
    void collapse(int x, int y) {
        auto& possible = grid[y][x];
        if (possible.empty()) { contradiction = true; return; }

        // Construir listas de tipos y pesos
        std::vector<int> types;
        std::vector<double> probs;
        for (int t : possible) {
            types.push_back(t);
            probs.push_back(weights[t]);
        }
        // Muestreo ponderado
        std::discrete_distribution<int> dist(probs.begin(), probs.end());
        int chosen = types[dist(rng)];

        possible.clear();
        possible.insert(chosen);
    }

    // Propagación BFS
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
                if (neighborSet.size() == 1) continue;

                std::set<int> valid;
                for (int nt : neighborSet) {
                    for (int ct : grid[cy][cx]) {
                        if (adjacency[ct][nt]) {
                            valid.insert(nt);
                            break;
                        }
                    }
                }

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

    // Ejecutar WFC completo
    bool run() {
        while (true) {
            auto [x, y] = getMinEntropyCell();
            if (contradiction) return false;
            if (x == -1) return true;

            collapse(x, y);
            if (contradiction) return false;
            propagate(x, y);
            if (contradiction) return false;
        }
    }

    // Obtener tesela final
    int getTile(int x, int y) const {
        if (grid[y][x].empty()) return -1;
        return *grid[y][x].begin();
    }

    // ================================================================
    // EXPORTACIÓN PPM (ahora en formato ASCII P3 por compatibilidad)
    // ================================================================
    void savePPM(const std::string& filename, int cellSize = 20, bool binary = false) const {
        int imgW = width  * cellSize;
        int imgH = height * cellSize;

        std::ofstream f(filename);
        if (!f.is_open()) {
            std::cerr << "Error: no se pudo abrir " << filename << "\n";
            return;
        }

        if (binary) {
            // Formato binario P6 (más compacto)
            f << "P6\n" << imgW << " " << imgH << "\n255\n";
            for (int py = 0; py < imgH; py++) {
                for (int px = 0; px < imgW; px++) {
                    int cx = px / cellSize;
                    int cy = py / cellSize;
                    int tile = getTile(cx, cy);
                    Color c = (tile >= 0) ? TILE_COLORS[tile] : Color{255, 0, 255};
                    // Línea de grilla
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
        } else {
            // Formato ASCII P3 (legible, compatible con muchos visores)
            f << "P3\n" << imgW << " " << imgH << "\n255\n";
            for (int py = 0; py < imgH; py++) {
                for (int px = 0; px < imgW; px++) {
                    int cx = px / cellSize;
                    int cy = py / cellSize;
                    int tile = getTile(cx, cy);
                    Color c = (tile >= 0) ? TILE_COLORS[tile] : Color{255, 0, 255};
                    bool isBorder = (cellSize > 5) &&
                                    (px % cellSize == 0 || py % cellSize == 0);
                    if (isBorder) {
                        c.r = (unsigned char)(c.r * 0.65);
                        c.g = (unsigned char)(c.g * 0.65);
                        c.b = (unsigned char)(c.b * 0.65);
                    }
                    f << (int)c.r << " " << (int)c.g << " " << (int)c.b << " ";
                }
                f << "\n";
            }
        }
        f.close();
        std::cout << "  Imagen guardada: " << filename
                  << " (" << imgW << "x" << imgH << " px)\n";
    }

    // Estadísticas
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
    // Parámetros
    int width    = (argc > 1) ? std::stoi(argv[1]) : 30;
    int height   = (argc > 2) ? std::stoi(argv[2]) : 30;
    unsigned int seed = (argc > 3)
                        ? (unsigned int)std::stoul(argv[3])
                        : (unsigned int)std::time(nullptr);
    int cellSize = (argc > 4) ? std::stoi(argv[4]) : 20;
    int ruleset  = (argc > 5) ? std::stoi(argv[5]) : 0;  // nuevo parámetro

    const int MAX_RETRIES = 30;

    std::cout << "=== Wave Function Collapse - Generador de Ciudad ===\n";
    std::cout << "Parametros: " << width << "x" << height
              << " | semilla base=" << seed
              << " | celda=" << cellSize << "px"
              << " | ruleset=" << ruleset << "\n\n";

    for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
        unsigned int currentSeed = seed + (unsigned int)attempt;
        WFC wfc(width, height, currentSeed, ruleset);

        // Opcional: ajustar pesos para favorecer ciertos tipos (ejemplo para ruleset=1)
        if (ruleset == 1) {
            // Para el parque con lago, queremos más agua y parque, menos edificios y calles
            double w[TILE_COUNT] = {2.0, 1.0, 0.2, 0.5, 2.5}; // agua y parque con más peso
            wfc.setWeights(w);
        }

        bool success = wfc.run();

        if (success) {
            std::cout << "Generacion exitosa (intento " << (attempt + 1) << ")\n";
            wfc.printStats(currentSeed);

            std::string fname = "output_" + std::to_string(width)
                              + "x" + std::to_string(height)
                              + "_seed" + std::to_string(currentSeed)
                              + "_ruleset" + std::to_string(ruleset)
                              + ".ppm";
            // Guardar en formato ASCII (P3) para compatibilidad con visores simples
            wfc.savePPM(fname, cellSize, false); // false = ASCII

            std::cout << "\nListo. Para visualizar: abre el archivo .ppm\n";
            std::cout << "con GIMP, IrfanView, o conviértelo a PNG:\n";
            std::cout << "  convert " << fname << " " << fname.substr(0, fname.size()-4) << ".png\n";
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