/******************************************************************************
 * Programacao Concorrente
 * MEEC 21/22
 *
 * Projecto - Parte1
 *                           serial-complexo.c
 * 
 * Compilacao: gcc serial-complexo -o serial-complex -lgd
 *           
 *****************************************************************************/

#include "gd.h"
#include "image-lib.h"
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

/* the directories wher output files will be placed */
#define RESIZE_DIR "/serial-Resize/"
#define THUMB_DIR "/serial-Thumbnail/"
#define WATER_DIR "/serial-Watermark/"

typedef struct img_t
{
	gdImagePtr watermark_image;
	gdImagePtr image;
	char* file_name;
	char* water_dir;
	char* resize_dir;
	char* thumb_dir;
}Image;

/******************************************************************************
 * modify_image (void *img_struct)
 *
 * Arguments: 
 *    void *img_struct (contém o ponteiro para a struct image_struct)
 * 
 * Returns: (void*)info
 *
 * Description: Aplica a marca de água á imagem, modifica o tamanho da imagem
 * e cria uma thumbnail da imagem original.
 *****************************************************************************/
void* modify_image(void *img_struct)
{
   Image *info = (Image*)img_struct;
   if(info == NULL){
	return (void*)info;
   }

	gdImagePtr out_watermark_img;

	/*Nome da imagem e a diretoria para onde esta vai ser copiada*/
	char *out_file_name = (char*)malloc((strlen(info->water_dir)+strlen(info->file_name)+1)*sizeof(char));
	sprintf(out_file_name, "%s%s", info->water_dir, info->file_name);

	/*---------------------------------------watermark---------------------------------------*/
	if(access(out_file_name, F_OK) == -1){ /*Imagem ainda não existe, por isso vai criá-la*/
		/*Aplica a watermark à imagem*/
		out_watermark_img = add_watermark(info->image, info->watermark_image);

		if (out_watermark_img == NULL) {
			fprintf(stderr, "Impossible to creat thumbnail of %s image\n", info->file_name);
		}
		else{
			printf("watermark  %s\n", info->file_name);
			/* save watermark */
			if(write_png_file(out_watermark_img, out_file_name) == 0){
				fprintf(stderr, "Impossible to write %s image\n", out_file_name);
			}
		}
	}
	else{ /*A imagem ja existe, logo passa à modificação seguinte*/
		printf("Imagem %s ja está presente na diretoria serial-watermark\n", info->file_name);
		out_watermark_img = add_watermark(info->image, info->watermark_image);
	}

	gdImageDestroy(info->image);

	/*-------------------------------------resize---------------------------------------*/
	sprintf(out_file_name, "%s%s", info->resize_dir, info->file_name);
	if(access(out_file_name, F_OK) == -1){ /*Se a imagem ainda não existe, então vamos criá-la*/
		gdImagePtr out_resized_img;

		out_resized_img = resize_image(out_watermark_img, 100); /*modificar o tamanho da imagem*/
		if (out_resized_img == NULL) {
			fprintf(stderr, "Impossible to resize %s image\n", info->file_name);
		}
		else{
			printf("resize  %s\n", info->file_name);
			/* save resized */
			if(write_png_file(out_resized_img, out_file_name) == 0){
				fprintf(stderr, "Impossible to write %s image\n", out_file_name);
			}
			gdImageDestroy(out_resized_img);
		}
	}
	else{ /*se ja existe a imagem então passamos à proxima modificação*/
		printf("Imagem %s ja está presente na diretoria serial-resize\n", info->file_name);
	}

	/*-------------------------------------thumbnail---------------------------------------*/
	sprintf(out_file_name, "%s%s", info->thumb_dir, info->file_name);
	if(access(out_file_name, F_OK) == -1){ /*Se a imagem ainda não existe, então vamos criá-la*/
		gdImagePtr out_thumb_img;

		out_thumb_img = make_thumb(out_watermark_img, 150); /*Criar a thumbnail*/
		if (out_thumb_img == NULL) {
			fprintf(stderr, "Impossible to creat thumbnail of %s image\n", info->file_name);
		}else{
			printf("thumbnail  %s\n", info->file_name);
			/* save thumbnail image */
			if(write_png_file(out_thumb_img, out_file_name) == 0){
				fprintf(stderr, "Impossible to write %s image\n", out_file_name);
			}
			gdImageDestroy(out_thumb_img);
		}
	}
	else{ /* Imagem ja existe */
		printf("Imagem %s ja está presente na diretoria serial-thumb\n", info->file_name);
	}

	gdImageDestroy(out_watermark_img);

	free(out_file_name);

	return (void*)info;
}


int main(int argc, char** argv)
{
    /*Número de imagens a processar*/
    int nn_files=0, n_threads = atoi(argv[2]), i=0, j=0, thread_index=0, wordlen=0, biggest_file_name=0;
	char *image_list = "/image-list.txt", *water_dir_s, *resize_dir_s, *thumb_dir_s;

	/*Nome do ficheiro onde estão os nomes das imagens a processar*/
	char* file_name = malloc((strlen(argv[1]) + strlen(image_list)+1) * sizeof(char)), *dir_name = argv[1];

	if(argc != 3){
		printf("Missing argument or too many arguments\n");
		exit(-1);
	}

	sprintf(file_name, "%s%s", dir_name, image_list);

	FILE *file_pointer = fopen(file_name, "r");
	if(file_pointer==NULL){
		exit(-1);
	}

	/*vetor com o nome das imagens a processar*/
	char **files; 

    /*String com o nome do ficheiro onde a imagem é guardada*/
	char buffer[100];

	while(fscanf(file_pointer, "%s", buffer)==1){ /*Primeira leitura do ficheiro para obter o número de imagens*/
		nn_files++;
	}

	fseek(file_pointer, 0, SEEK_SET); /*colocar o ponteiro do ficheiro no inicio*/

	if(n_threads>nn_files){
		files = (char**)malloc(n_threads*sizeof(char*)); /*Alocar espaço para o vetor com o nome dos ficheiros*/
		if(files == (char**)NULL){
			exit(0);
		}
	}
	else{
		files = (char**)malloc(nn_files*sizeof(char*)); /*Alocar espaço para o vetor com o nome dos ficheiros*/
		if(files == (char**)NULL){
			exit(0);
		}
	}

	nn_files=0;
	while(fscanf(file_pointer, "%s", buffer)==1){ /*escreve os nomes das imagens na variável files*/
		wordlen=strlen(buffer);
		if(wordlen>biggest_file_name){ /*Para encontrar a imagem com o maior nome para ajudar na alocação de memória*/
			biggest_file_name = wordlen;
		}
		files[nn_files] = (char*)malloc((strlen(dir_name)+wordlen+2) * sizeof(char));
		sprintf(files[nn_files], "%s", buffer);
		nn_files++;
	}

	fclose(file_pointer);

	/*Cálculo para a distribuição de imagens pelas threads*/
	int images_per_thread = nn_files/n_threads; /*Esta variável vai conter o número de imagens por thread inicialmente*/
	if(images_per_thread==0){
		images_per_thread=1;
	}
	int images_left = nn_files-(images_per_thread*n_threads); /*Esta variável contém o número de imagens que sobram no final*/
	if(images_left<0){
		images_left=0;
	}

	/*vetor com o id das threads selecionadas como argumento*/
	pthread_t thread_id[n_threads];

	/*Criação da estrutura para a marca de água*/
	Image *image_struct[n_threads];

	for(int i=0; i<n_threads; i++){
		image_struct[i] = (Image*)calloc(1,sizeof(Image));
	}

    /*Pointers para as imagens iniciais*/
	gdImagePtr watermark_img;

    /*Criação das diretorias de output para guardar as imagens*/
	int dir_name_len = strlen(dir_name);
	resize_dir_s = malloc((dir_name_len + 17) * sizeof(char));
	water_dir_s = malloc((dir_name_len + 20) * sizeof(char));
	thumb_dir_s = malloc((dir_name_len + 20) * sizeof(char));

	sprintf(resize_dir_s, "%s%s", dir_name, RESIZE_DIR);
	sprintf(thumb_dir_s, "%s%s", dir_name, THUMB_DIR);
	sprintf(water_dir_s, "%s%s", dir_name, WATER_DIR);

	/*Criação das diretorias*/
	if (create_directory(resize_dir_s) == 0){
		fprintf(stderr, "Impossible to create %s directory\n", resize_dir_s);
		exit(-1);
	}
	if (create_directory(thumb_dir_s) == 0){
		fprintf(stderr, "Impossible to create %s directory\n", thumb_dir_s);
		exit(-1);
	}
	if (create_directory(water_dir_s) == 0){
		fprintf(stderr, "Impossible to create %s directory\n", water_dir_s);
		exit(-1);
	}

    /*Importar a watermark para um ponteiro*/
	char* watermark_png = "watermark.png";
	char* watermark_name = malloc((strlen(watermark_png) + strlen(dir_name) + 2) * sizeof(char));

	/*string com o nome da imagem de watermark na sua respetiva diretoria*/
	sprintf(watermark_name, "%s/%s", dir_name, watermark_png);
    watermark_img = read_png_file(watermark_name);
	if(watermark_img == NULL){
		fprintf(stderr, "Impossible to read %s image\n", "watermark.png");
		exit(-1);
	}

	for(int i=0; i<n_threads;i++){ /*Inicialização das variáveis da struct usada nas threads*/
		image_struct[i]->watermark_image = watermark_img;
		image_struct[i]->resize_dir = resize_dir_s;
		image_struct[i]->water_dir = water_dir_s;
		image_struct[i]->thumb_dir = thumb_dir_s;
		image_struct[i]->file_name = (char*)malloc((biggest_file_name + 1) * sizeof(char));
	}

	/*Variável com o nome da imagem dentro da thread*/
	char* file_in_dir = malloc((strlen(dir_name) + biggest_file_name + 2)* sizeof(char));

	/*Distribuição das imagens pelas threads*/

	while(i<images_per_thread){ /*Primeira distribuição das imagens pelas threads*/
		while(j<n_threads){
			if(j>=nn_files){
				pthread_create(&thread_id[j], NULL, modify_image, (void*)NULL);
				j++;
				continue;;
			}
			sprintf(file_in_dir, "%s/%s", dir_name, files[thread_index]);
			image_struct[j]->image = read_png_file(file_in_dir);
			strcpy(image_struct[j]->file_name, files[thread_index]);
			pthread_create(&thread_id[j], NULL, modify_image, (void*)image_struct[j]);
			thread_index++;
			j++;
		}
		j=0;
		while(j<n_threads){ /*Juntar as threads que foram criadas até agora*/
			pthread_join(thread_id[j], NULL);
			j++;
		}
		j=0;
		i++;
	}	

	/*Distribuir as restantes imagens pelas ultimas threads*/
	for(int i=0; i<images_left; i++){
		sprintf(file_in_dir, "%s/%s", dir_name, files[thread_index]);
		image_struct[i]->image = read_png_file(file_in_dir);
		strcpy(image_struct[i]->file_name, files[thread_index]);
		pthread_create(&thread_id[i], NULL, modify_image, (void*)image_struct[i]);
		thread_index++;
	}
	for(int i=0; i<images_left; i++){
		pthread_join(thread_id[i], NULL);
	}

	/*Libertação de memória*/
	for(int i=0; i<n_threads; i++){
		free(image_struct[i]->file_name);
		free(image_struct[i]);

	}
	free(watermark_name);
	free(file_in_dir);
	free(water_dir_s);
	free(thumb_dir_s);
	free(resize_dir_s);
	free(file_name);
	gdImageDestroy(watermark_img);

	for(int i=0; i<nn_files; i++){
		free(files[i]);
	}
	free(files);

	return 0;
}