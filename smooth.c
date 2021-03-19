// Aluno: Víctor Cora Colombo
// RA: 727356
// Curso: Engenharia de Computação
// Matéria: PROGRAMAÇÃO PARALELA E DISTRIBUÍDA

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

// comentario do aluno:
// adaptei para ser +2 para ter uma linha/coluna a mais antes e depois
// isso significa que utilizarei a abordagem de ter linhas e colunas dummy
// ao inves de usar ifs
#define WIDTH 512 + 2
#define HEIGHT 512 + 2

// comentario do aluno:
// número de threads. Valores muito altos podem causar degradação da performance
// pelo processo ser cpu-bound, o ideal provavelmente é N_THREAD = cores na máquina
#define N_THREADS 4
pthread_t threads[N_THREADS];

// comentario do aluno:
// invertir HEIGHT E WIDTH aqui
int mr[HEIGHT][WIDTH];
int mg[HEIGHT][WIDTH];
int mb[HEIGHT][WIDTH];
int ma[HEIGHT][WIDTH];
int mr2[HEIGHT][WIDTH];
int mg2[HEIGHT][WIDTH];
int mb2[HEIGHT][WIDTH];
int ma2[HEIGHT][WIDTH];

// comentários do aluno:
// calcula a média de matriz[i][j] com seus vizinhos imediatos
// teria de ser refatorado para uma versão do algoritmo smooth com stencil maior
// condições esperadas:
// 0 < i <= nrow, 0 < j <= ncol
int mediaSmooth(int matriz[HEIGHT][WIDTH], int i, int j)
{
	int s = 0;
	// 1a linha
	s += matriz[i - 1][j - 1];
	s += matriz[i - 1][j];
	s += matriz[i - 1][j + 1];
	// 2a linha
	s += matriz[i][j - 1];
	s += matriz[i][j];
	s += matriz[i][j + 1];
	// 3a linha
	s += matriz[i + 1][j - 1];
	s += matriz[i + 1][j];
	s += matriz[i + 1][j + 1];

	// são 9 pontos, então a média é dividir por 9
	return s / 9;
}

// comentario do aluno:
// Esse método funciona da seguinte forma:
// estratégia de paralelização por dados. Cada thread é responsável pela linha i == thread_number % N_THREADS
// por exemplo: na thread de thread_number 1, ela é responsável pelas linhas 1, 9... (dado N_THREADS = 8)
// enquanto a thread 2 é responsável por 2, 10...
// e a linha 0? ela é dummy
// problemas dessa escolha: as linhas "vizinhas" estão sendo cacheadas mas são subutilizadas
// pois elas são utilizadas apenas na média
void *threadSmoothAlternarLinhas(void *thread_number_void_ptr)
{
	struct timeval fim, inic;
	gettimeofday(&inic, NULL);

	int thread_number = *(int *)thread_number_void_ptr + 1;
	free(thread_number_void_ptr);

	// aplicar filtro (estêncil)
	// repetir para mr2, mg2, mb2, ma2
	int nlin, ncol, i, j;
	nlin = HEIGHT - 1;
	ncol = WIDTH - 1;
	// tratar: linhas 0, 1, n, n-1; colunas 0,1,n,n-1
	for (i = thread_number; i < nlin; i += N_THREADS)
	{
		for (j = 1; j < ncol; j++)
		{
			mr2[i][j] = mediaSmooth(mr, i, j);
			mg2[i][j] = mediaSmooth(mg, i, j);
			mb2[i][j] = mediaSmooth(mb, i, j);
			ma2[i][j] = mediaSmooth(ma, i, j);
		}
	}

	gettimeofday(&fim, NULL);

	double *ret = malloc(sizeof(double));
	*ret = (fim.tv_sec + fim.tv_usec / 1000000.) - (inic.tv_sec + inic.tv_usec / 1000000.);
	pthread_exit(ret);
}

// comentario do aluno:
// esse método busca aproveitar melhor a cache
// linhas "vizinhas" são reutilizadas pois cada thread é responsável por um bloco de linhas
// por exemplo a thread 1 é responsável pelas linhas 1,2,3...HEIGHT/N_THREADS. Assim, linhas vizinhas
// cacheadas são reutilizadas pela thread
void *threadSmoothBlocosDeLinhas(void *thread_number_void_ptr)
{
	struct timeval fim, inic;
	gettimeofday(&inic, NULL);

	int thread_number = *(int *)thread_number_void_ptr;
	free(thread_number_void_ptr);

	// aplicar filtro (estêncil)
	// repetir para mr2, mg2, mb2, ma2
	int nlin, ncol, i, j;
	nlin = HEIGHT - 1;
	ncol = WIDTH - 1;
	int begin = nlin / N_THREADS * thread_number;
	int end = begin + nlin / N_THREADS;
	// tratar: linhas 0, 1, n, n-1; colunas 0,1,n,n-1
	for (i = begin; i <= end; i++)
	{
		for (j = 1; j < ncol; j++)
		{
			mr2[i][j] = mediaSmooth(mr, i, j);
			mg2[i][j] = mediaSmooth(mg, i, j);
			mb2[i][j] = mediaSmooth(mb, i, j);
			ma2[i][j] = mediaSmooth(ma, i, j);
		}
	}

	gettimeofday(&fim, NULL);

	double *ret = malloc(sizeof(double));
	*ret = (fim.tv_sec + fim.tv_usec / 1000000.) - (inic.tv_sec + inic.tv_usec / 1000000.);
	pthread_exit(ret);
}

void criarThreads()
{
	int i;
	for (i = 0; i < N_THREADS; i++)
	{
		int *arg = malloc(sizeof(i));
		*arg = i;
		pthread_create(&(threads[i]), NULL, threadSmoothBlocosDeLinhas, arg);
	}
}

// comentarios do aluno:
// aguarda o retorno das threads e calcula o tempo de execução de cada uma
void joinThreads()
{
	double max = -1, min = 9999999;
	int i;
	for (i = 0; i < N_THREADS; i++)
	{
		double *resposta;
		pthread_join(threads[i], (void *)&resposta);
		printf("Thread %d : %f sec\n", i, *resposta);
		max = *resposta > max ? *resposta : max;
		min = *resposta < min ? *resposta : min;
		free(resposta);
	}
	printf("Tempo mínimo de execução de uma thread: %f sec\n", min);
	printf("Tempo máximo de execução de uma thread: %f sec\n", max);
}

// comentarios do aluno:
// essa função abre a imagem original, carrega-a numa matriz,
// e inicializa a matriz que guardará o resultado do smooth
void inicializarMatrizes(char *name)
{
	int nlin, ncol, i, j, fdi;

	if ((fdi = open(name, O_RDONLY)) == -1)
	{
		printf("Erro na abertura do arquivo %s\n", name);
		exit(0);
	}

	// zerar as matrizes (4 bytes, mas usaremos 1 por pixel)
	// void *memset(void *s, int c, size_t n);
	memset(mr, 0, HEIGHT * WIDTH * sizeof(int));
	memset(mg, 0, HEIGHT * WIDTH * sizeof(int));
	memset(mb, 0, HEIGHT * WIDTH * sizeof(int));
	memset(ma, 0, HEIGHT * WIDTH * sizeof(int));
	memset(mr2, 0, HEIGHT * WIDTH * sizeof(int));
	memset(mg2, 0, HEIGHT * WIDTH * sizeof(int));
	memset(mb2, 0, HEIGHT * WIDTH * sizeof(int));
	memset(ma2, 0, HEIGHT * WIDTH * sizeof(int));

	// (ao menos) 2 abordagens:
	// - ler pixels byte a byte, colocando-os em matrizes separadas
	//	- ler pixels (32bits) e depois usar máscaras e rotações de bits para o processamento.
	// comentario do aluno:
	// abordagem utilizada foi a de colocar os pixeis em matrizes separadas

	// ordem de leitura dos bytes (componentes do pixel) depende se o formato
	// é little ou big endian
	// Assumindo little endian
	nlin = HEIGHT - 1;
	ncol = WIDTH - 1;
	for (i = 1; i < nlin; i++)
	{
		for (j = 1; j < ncol; j++)
		{
			read(fdi, &mr[i][j], 1);
			read(fdi, &mg[i][j], 1);
			read(fdi, &mb[i][j], 1);
			read(fdi, &ma[i][j], 1);
		}
	}

	// copiar as bordas para evitar a borda preta
	// copiar borda superior
	for (j = 1; j < ncol; j++)
	{
		mr[0][j] = mr[1][j];
		mg[0][j] = mg[1][j];
		mb[0][j] = mb[1][j];
		ma[0][j] = ma[1][j];
	}
	// copiar borda direita
	for (i = 1; i < nlin; i++)
	{
		mr[i][ncol] = mr[i][ncol - 1];
		mg[i][ncol] = mg[i][ncol - 1];
		mb[i][ncol] = mb[i][ncol - 1];
		ma[i][ncol] = ma[i][ncol - 1];
	}
	// copiar borda inferior
	for (j = 1; j < ncol; j++)
	{
		mr[nlin][j] = mr[nlin - 1][j];
		mg[nlin][j] = mg[nlin - 1][j];
		mb[nlin][j] = mb[nlin - 1][j];
		ma[nlin][j] = ma[nlin - 1][j];
	}
	// copiar borda esquerda
	for (i = 1; i < nlin; i++)
	{
		mr[i][0] = mr[i][0];
		mg[i][0] = mg[i][0];
		mb[i][0] = mb[i][0];
		ma[i][0] = ma[i][0];
	}

	close(fdi);
}

// salva a nova imagem
void escreverMatrizes(char *name)
{
	int nlin, ncol, i, j, fdo;

	fdo = open(name, O_WRONLY | O_CREAT);
	nlin = HEIGHT - 1;
	ncol = WIDTH - 1;
	for (i = 1; i < nlin; i++)
	{
		for (j = 1; j < ncol; j++)
		{
			write(fdo, &mr2[i][j], 1);
			write(fdo, &mg2[i][j], 1);
			write(fdo, &mb2[i][j], 1);
			write(fdo, &ma2[i][j], 1);
		}
	}
	close(fdo);
}

int main(int argc, char **argv)
{
	char name[128];

	if (argc < 2)
	{
		printf("Uso: %s nome_arquivo\n", argv[0]);
		exit(0);
	}

	inicializarMatrizes(argv[1]);

	struct timeval fim, inic;
	gettimeofday(&inic, NULL);

	criarThreads();
	joinThreads();

	gettimeofday(&fim, NULL);
	printf("Tempo total de execução (desconsiderando leitura/inicialização e escrita das matrizes): %f sec\n", (fim.tv_sec + fim.tv_usec / 1000000.) - (inic.tv_sec + inic.tv_usec / 1000000.));
	// resultados:
	// sequencial: 0.017315 segundos = 17.315 ms
	// paralelo (com threadSmoothAlternarLinhas): 0.007590 segundos = 7.590 ms
	// paralelo (com threadSmoothBlocosDeLinhas): 0.005845 segundos = 5.845 ms
	// dessa forma, o speedup é de (considerando speedup = antes/depois):
	// speedup = 17.315 / 5.845 = 2.96
	// Ou seja, o paralelismo usando threadSmoothBlocosDeLinhas é quase 3 vezes mais rápido

	sprintf(name, "%s.new", argv[1]);
	escreverMatrizes(name);

	// comentarios adicionais:
	// em ambos os casos de paralelização foi usado a estratégia de paralelização por dados
	//
	// outra opção possível seria a paralelização funcional
	// nesse caso, as outras operações presentes nesse código são a leitura e escrita dos dados
	// a paralelização dessas tarefas é ruim, pois no Linux o vetor de descritores de arquivo
	// é compartilhado. Dessa forma, não é possível ter o mesmo arquivo "aberto duas vezes", o
	// que seria necessário para paralelizar a tarefa. Há um jeito de "burlar" isso, usando
	// uma leitura via pread. Mas o disco só consegue acessar uma posição por vez, então na realidade
	// a tarefa acabaria sendo serializada fora do código de qualquer forma.
	//
	// outra ideia possível (nesse caso, por dados), seria paralelizar o cálculo da média. Não testei essa opção,
	// mas me parece que seria overkill paralelizar uma tarefa que é a simples soma de 9 elementos. O overhead
	// das threads provavelmente seria muito pior que qualquer ganho possível.

	return 0;
}

// script simples para executar rapidamente o código:
// execute.sh
// #!/bin/bash
//
// # nao esquecer de passar a imagem como argumento
// gcc paralelo.c -o smooth -lpthread
// ./smooth $1
// chmod 644 $1.new
// convert -size 512x512 -depth 8 rgba:$1.new $1_new.tiff
// rm $1.new
// # trocar gwenview pelo seu visualizador de imagens
// gwenview $1_new.tiff

// minha implementação sequencial:

// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <sys/time.h>
// #include <fcntl.h>
// #include <unistd.h>

// // comentario do aluno:
// // somei +2 em WIDTH e HEIGHT para adicionar linhas e colunas dummy
// // #define WIDTH  7680
// #define WIDTH 512 + 2
// // #define HEIGHT 4320
// #define HEIGHT 512 + 2

// // comentario do aluno:
// // invertir HEIGHT E WIDTH aqui
// int mr[HEIGHT][WIDTH];
// int mg[HEIGHT][WIDTH];
// int mb[HEIGHT][WIDTH];
// int ma[HEIGHT][WIDTH];
// int mr2[HEIGHT][WIDTH];
// int mg2[HEIGHT][WIDTH];
// int mb2[HEIGHT][WIDTH];
// int ma2[HEIGHT][WIDTH];

// // comentários do aluno:
// // condições esperadas:
// // 0 < i <= nrow, 0 < j <= ncol
// int mediaSmooth(int matriz[HEIGHT][WIDTH], int i, int j)
// {
// 	int s = 0;
// 	// 1a linha
// 	s += matriz[i - 1][j - 1];
// 	s += matriz[i - 1][j];
// 	s += matriz[i - 1][j + 1];
// 	// 2a linha
// 	s += matriz[i][j - 1];
// 	s += matriz[i][j];
// 	s += matriz[i][j + 1];
// 	// 3a linha
// 	s += matriz[i + 1][j - 1];
// 	s += matriz[i + 1][j];
// 	s += matriz[i + 1][j + 1];

// 	return s / 9;
// }

// int main(int argc, char **argv)
// {
// 	int i, j;
// 	int fdi, fdo;
// 	int nlin = 0;
// 	int ncol = 0;
// 	char name[128];

// 	if (argc < 2)
// 	{
// 		printf("Uso: %s nome_arquivo\n", argv[0]);
// 		exit(0);
// 	}
// 	if ((fdi = open(argv[1], O_RDONLY)) == -1)
// 	{
// 		printf("Erro na abertura do arquivo %s\n", argv[1]);
// 		exit(0);
// 	}

// 	nlin = ncol = 512;

// 	// zerar as matrizes (4 bytes, mas usaremos 1 por pixel)
// 	// void *memset(void *s, int c, size_t n);
// 	memset(mr, 0, HEIGHT * WIDTH * sizeof(int));
// 	memset(mg, 0, HEIGHT * WIDTH * sizeof(int));
// 	memset(mb, 0, HEIGHT * WIDTH * sizeof(int));
// 	memset(ma, 0, HEIGHT * WIDTH * sizeof(int));
// 	memset(mr2, 0, HEIGHT * WIDTH * sizeof(int));
// 	memset(mg2, 0, HEIGHT * WIDTH * sizeof(int));
// 	memset(mb2, 0, HEIGHT * WIDTH * sizeof(int));
// 	memset(ma2, 0, HEIGHT * WIDTH * sizeof(int));

// 	// comentario do aluno:
// 	// adaptei para ser +2 para ter uma linha/coluna a mais antes e depois
// 	// isso significa que utilizarei a abordagem de ter linhas e colunas dummy
// 	// ao inves de usar ifs

// 	// (ao menos) 2 abordagens:
// 	// - ler pixels byte a byte, colocando-os em matrizes separadas
// 	//	- ler pixels (32bits) e depois usar máscaras e rotações de bits para o processamento.
// 	// comentario do aluno:
// 	// abordagem utilizada foi a de colocar os pixeis em matrizes separadas

// 	// ordem de leitura dos bytes (componentes do pixel) depende se o formato
// 	// é little ou big endian
// 	// Assumindo little endian
// 	for (i = 0; i < nlin; i++)
// 	{
// 		for (j = 0; j < ncol; j++)
// 		{
// 			read(fdi, &mr[i][j], 1);
// 			read(fdi, &mg[i][j], 1);
// 			read(fdi, &mb[i][j], 1);
// 			read(fdi, &ma[i][j], 1);
// 		}
// 	}
// 	close(fdi);

// 	struct timeval fim, inic;
// 	gettimeofday(&inic, NULL);

// 	// aplicar filtro (estêncil)
// 	// repetir para mr2, mg2, mb2, ma2

// 	// tratar: linhas 0, 1, n, n-1; colunas 0,1,n,n-1
// 	for (i = 1; i <= nlin; i++)
// 	{
// 		for (j = 1; j <= ncol; j++)
// 		{
// 			mr2[i][j] = mediaSmooth(mr, i, j);
// 			mg2[i][j] = mediaSmooth(mg, i, j);
// 			mb2[i][j] = mediaSmooth(mb, i, j);
// 			ma2[i][j] = mediaSmooth(ma, i, j);
// 		}
// 	}

// 	gettimeofday(&fim, NULL);
// 	printf("Tempo total de execução (desconsiderando leitura/inicialização das matrizes e escrita): %f\n", (fim.tv_sec + fim.tv_usec / 1000000.) - (inic.tv_sec + inic.tv_usec / 1000000.));

// 	// gravar imagem resultante
// 	sprintf(name, "%s.new", argv[1]);
// 	fdo = open(name, O_WRONLY | O_CREAT);

// 	for (i = 1; i <= nlin; i++)
// 	{
// 		for (j = 1; j <= ncol; j++)
// 		{
// 			write(fdo, &mr2[i][j], 1);
// 			write(fdo, &mg2[i][j], 1);
// 			write(fdo, &mb2[i][j], 1);
// 			write(fdo, &ma2[i][j], 1);
// 		}
// 	}
// 	close(fdo);

// 	return 0;
// }
