# Suavização de pixeis (smooth) paralelo

Implementação de diversas estratégias de paralelização do algoritmo smooth para suavização de pixeis de forma paralela com _pthreads_.

## Como usar

A imagem precisa ser rgba 512x512

`./execute nome_da_image.rgba`

Para modificar o tamanho da imagem, vá em `smooth.c` e modifique as linhas:

```c
#define WIDTH 512 + 2
#define HEIGHT 512 + 2
```

trocando 512 para o valor desejado. Pode haver problemas se o tamanho não for uma potência de 2.

## Exemplos

Antes:

![Lena antes](images/lena_noise_original.tiff)

![Lena depois](images/lena_noise.rgba_new.tiff)

![Noisy antes](images/noisyimg_original.tiff)

![Noisy depois](images/noisyimg.rgba_new.tiff)
