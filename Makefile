CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
TARGET   = wfc_city

# Parámetros por defecto para 'make run'
WIDTH    = 30
HEIGHT   = 30
SEED     = 42
CELLSIZE = 20
RULESET  = 0   # 0=original, 1=parque con lago, 2=estricta

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp

# Ejecutar con parámetros por defecto (ruleset 0)
run: $(TARGET)
	./$(TARGET) $(WIDTH) $(HEIGHT) $(SEED) $(CELLSIZE) $(RULESET)

# Generar varios mapas con distintos rulesets y semillas
batch: $(TARGET)
	./$(TARGET) 30 30 42  20 0
	./$(TARGET) 30 30 100 20 0
	./$(TARGET) 30 30 999 20 0
	./$(TARGET) 20 20 42  20 0
	./$(TARGET) 40 40 42  20 0
	# Ahora con ruleset 1 (parque con lago)
	./$(TARGET) 30 30 42  20 1
	./$(TARGET) 30 30 100 20 1
	./$(TARGET) 30 30 999 20 1
	./$(TARGET) 20 20 42  20 1
	./$(TARGET) 40 40 42  20 1
	# Y con ruleset 2 (estricta)
	./$(TARGET) 30 30 42  20 2
	./$(TARGET) 30 30 100 20 2
	./$(TARGET) 30 30 999 20 2
	./$(TARGET) 20 20 42  20 2
	./$(TARGET) 40 40 42  20 2

# Conversión automática de todos los .ppm a .png (requiere ImageMagick)
convert: batch
	for ppm in output_*.ppm; do \
		png=$${ppm%.ppm}.png; \
		convert $$ppm $$png; \
	done

clean:
	rm -f $(TARGET) *.ppm *.png

.PHONY: all run batch clean convert