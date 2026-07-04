# Wave Function Collapse - Generador de Ciudad

Implementación en C++17 del algoritmo Wave Function Collapse para generación procedural de mapas urbanos 2D, inspirada en Townscaper.

## Requisitos

- Compilador C++17 (g++ recomendado)
- `make` (opcional, para usar el Makefile)
- ImageMagick (opcional, para convertir imágenes PPM a PNG)

## Compilación

```bash
make
```

## Uso

```bash
./wfc_city [ancho] [alto] [semilla] [pixeles_por_celda] [ruleset]
```

| Parámetro          | Descripción                                                                 |
|--------------------|-----------------------------------------------------------------------------|
| `ancho`            | Número de celdas en horizontal (default: 30)                                |
| `alto`             | Número de celdas en vertical (default: 30)                                  |
| `semilla`          | Semilla aleatoria para reproducibilidad (default: time)                     |
| `pixeles_por_celda`| Tamaño en píxeles de cada celda en la imagen (default: 20)                  |
| `ruleset`          | Conjunto de reglas de adyacencia: 0=original, 1=parque con lago, 2=estricta (default: 0) |

### Ejemplo

```bash
./wfc_city 30 30 42 20 1
```

Genera un mapa de 30×30, semilla 42, con reglas de "parque con lago".

## Salida

El programa genera un archivo **PPM en formato ASCII (P3)** con el nombre:

```
output_<ancho>x<alto>_seed<semilla>_ruleset<ruleset>.ppm
```

Para visualizar la imagen, se recomienda convertir a PNG usando ImageMagick:

```bash
convert archivo.ppm archivo.png
```

## Makefile

- `make` → compila el ejecutable.
- `make run` → ejecuta con parámetros por defecto (30×30, semilla 42, ruleset 0).
- `make batch` → genera 15 mapas combinando tamaños (20,30,40), semillas (42,100,999) y rulesets (0,1,2).
- `make clean` → elimina ejecutable y archivos generados.

## Repositorio

[https://github.com/Pasita24/Tarea4-InteligenciaArtifical]

