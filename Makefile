CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
TARGET   = wfc_city

# Parámetros por defecto para 'make run'
WIDTH    = 30
HEIGHT   = 30
SEED     = 42
CELLSIZE = 20

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp

# Ejecutar con parámetros por defecto
run: $(TARGET)
	./$(TARGET) $(WIDTH) $(HEIGHT) $(SEED) $(CELLSIZE)

# Generar varios mapas con distintas semillas (para la sección de Resultados)
batch: $(TARGET)
	./$(TARGET) 30 30 42  20
	./$(TARGET) 30 30 100 20
	./$(TARGET) 30 30 999 20
	./$(TARGET) 20 20 42  20
	./$(TARGET) 40 40 42  20

clean:
	rm -f $(TARGET) *.ppm

.PHONY: all run batch clean
