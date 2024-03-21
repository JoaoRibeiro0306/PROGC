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
#include <unistd.h>

/* the directories wher output files will be placed */
#define RESIZE_DIR "/serial-Resize/"
#define THUMB_DIR "/serial-Thumbnail/"
#define WATER_DIR "/serial-Watermark/"

typedef struct Image_t
{
	gdImagePtr watermark_image;
	gdImagePtr image;
	char* file_name;
	char* directory;
}Image;

/******************************************************************************
 * watermark_image_function (void *img_struct)
 *
 * Arguments: 
 *    void *img_struct (contém o ponteiro para a struct image_struct)
 * 
 * Returns: (void*)info
 *
 * Description: Aplica a marca de água á imagem
 *****************************************************************************/
void* watermark_image_function(void *img_struct)
{
   Image *info = (Image*)img_struct;
	if(info == (Image*)NULL){
		return (void*)info;
	}

   char *out_file_name = (char*)malloc((strlen(info->file_name) + strlen(info->directory) + 1) * sizeof(char));
   sprintf(out_file_name, "%s%s", info->directory, info->file_name); /*nome da imagem na diretoria onde vai ser guardada*/

   if(access(out_file_name, F_OK) == -1){ /*Se a imagem ainda não existir cria*/
		gdImagePtr out_watermark_img;

		out_watermark_img = add_watermark(info->image, info->watermark_image); /*Aplicar watermark na imagem*/

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
		gdImageDestroy(out_watermark_img);
    }
    else{ /*Imagem ja foi criada logo passa à frente*/
		printf("Imagem %s ja está presente na diretoria serial-watermark\n", info->file_name);
	}	

	gdImageDestroy(info->image); 
	free(out_file_name);	

	return (void*)info;


}

/******************************************************************************
 * resize_image_function (void *img_struct)
 *
 * Arguments: 
 *    void *img_struct (contém o ponteiro para a struct image_struct)
 * 
 * Returns: (void*)info
 *
 * Description: Modifica o tamanho da imagem
 *****************************************************************************/
void* resize_image_function(void *img_struct)   
{  
    Image *info = (Image*)img_struct;
	if(info == (Image*)NULL){
		return (void*)info;
	}

    char *out_file_name = (char*)malloc((strlen(info->file_name) + strlen(info->directory) + 1) * sizeof(char));
	sprintf(out_file_name, "%s%s", info->directory, info->file_name); /*nome da imagem na diretoria onde vai ser guardada*/

	if(access(out_file_name, F_OK) == -1){ /*Se a imagem ainda não existir cria*/
		gdImagePtr out_resized_img, out_watermark_img;

		out_watermark_img = add_watermark(info->image, info->watermark_image); /*Aplicar watermark na imagem*/

		out_resized_img = resize_image(out_watermark_img, 100); /* Altera o tamanho da imagem*/
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
		gdImageDestroy(out_watermark_img);
	}
	else{ /*Imagem ja foi criada logo passa à frente*/
		printf("Imagem %s ja está presente na diretoria serial-resize\n", info->file_name);
	}	

	gdImageDestroy(info->image);
	free(out_file_name);

    return (void*)info;
 
}

/******************************************************************************
 * thumb_image_function (void *img_struct)
 *
 * Arguments: 
 *    void *img_struct (contém o ponteiro para a struct image_struct)
 * 
 * Returns: (void*)info
 *
 * Description: Cria uma thumbnail da imagem original
 *****************************************************************************/
void* thumb_image_function(void *img_struct)

{
    Image *info = (Image*)img_struct;
	if(info == (Image*)NULL){
		return (void*)info;
	}

    char *out_file_name = (char*)malloc((strlen(info->file_name) + strlen(info->directory) + 1) * sizeof(char));
	sprintf(out_file_name, "%s%s", info->directory, info->file_name); /*nome da imagem na diretoria onde vai ser guardada*/

	if(access(out_file_name, F_OK) == -1){ /*Se a imagem ainda não existir cria*/
		gdImagePtr out_thumb_img, out_watermark_img;

		out_watermark_img = add_watermark(info->image, info->watermark_image); /*Aplicar watermark na imagem*/
	
		out_thumb_img = make_thumb(out_watermark_img, 100); /*Cria a thumbnail da imagem original*/
		if (out_thumb_img == NULL) {
			fprintf(stderr, "Impossible to creat thumbnail of %s image\n", info->file_name);
		}
		else{
			printf("thumbnail  %s\n", info->file_name);
			/* save thumbnail image */
			if(write_png_file(out_thumb_img, out_file_name) == 0){
				fprintf(stderr, "Impossible to write %s image\n", out_file_name);
			}
			gdImageDestroy(out_thumb_img);
		}
		gdImageDestroy(out_watermark_img);
	}
	else{ /*Imagem ja foi criada logo passa à frente*/
		printf("Imagem %s ja está presente na diretoria serial-thumb\n", info->file_name);
	}	

	free(out_file_name);
	gdImageDestroy(info->image);

	return (void*)info;
}

/******************************************************************************
 * int main (int argc, char** argv)
 *
 * Arguments: 
 *    int argc (número de argumentos à chamada do programa)
 * 	  char** argv (argumentos à chamada do programa)	
 * 
 * Returns: 0
 *
 * Description: Cria as threads e envia as imagens para as threads para serem
 * processadas
 *****************************************************************************/
int main(int argc, char** argv)
{
    /*Número de imagens a processar*/
    int nn_files=0, wordlen=0, biggest_file_name=0;
	char *file_name, *dir_name = argv[1], *image_list_s = "/image-list.txt";
	file_name = malloc((strlen(dir_name) + strlen(image_list_s) + 1)* sizeof(char));
	sprintf(file_name, "%s%s", dir_name, image_list_s);

	FILE *file_pointer = fopen(file_name, "r");
	if(file_pointer == NULL){
		printf("Couldnt open file");
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

	files = (char**)malloc(nn_files*sizeof(char*)); /*Alocar espaço para o vetor com o nome dos ficheiros*/
	if(files == (char**)NULL){
        exit(0);
    }

	nn_files=0;
	while(fscanf(file_pointer, "%s", buffer)==1){ /*Ler cada palavra do ficheiro e guardá-la no vetor files*/
		wordlen=strlen(buffer);
		if(wordlen>biggest_file_name){ /*Descobrir a maior palavra do ficheiro*/
			biggest_file_name = wordlen;
		}
		files[nn_files] = (char*)malloc((wordlen+1) * sizeof(char));
		strcpy(files[nn_files], buffer);
		nn_files++;
	}

	fclose(file_pointer);

	pthread_t thread_id[3];

	
	/*Criação da estrutura para a marca de água*/
	Image *image_struct[3];
	for(int i=0; i<3; i++){
		image_struct[i] = (Image*)malloc(1*sizeof(Image));
	}


	/*Pointers para as imagens iniciais*/
	gdImagePtr watermark_img;
	
	char *water_dir_s, *resize_dir_s, *thumb_dir_s;
	int dir_name_len = strlen(dir_name);
	resize_dir_s = malloc((dir_name_len + 17) * sizeof(char));
	water_dir_s = malloc((dir_name_len + 20) * sizeof(char));
	thumb_dir_s = malloc((dir_name_len + 20) * sizeof(char));

	/*Criação das strings do nome das diretorias*/
	sprintf(resize_dir_s, "%s%s", dir_name, RESIZE_DIR);
	sprintf(thumb_dir_s, "%s%s", dir_name, THUMB_DIR);
	sprintf(water_dir_s, "%s%s", dir_name, WATER_DIR);

    /*Criação das diretorias de output para guardar as imagens*/
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
	sprintf(watermark_name, "%s/%s", dir_name, watermark_png);

	/*Criar o pointer para a imagem que vai ser a watermark*/
    watermark_img = read_png_file(watermark_name);
	if(watermark_img == NULL){
		fprintf(stderr, "Impossible to read %s image\n", "watermark.png");
		exit(-1);
	}

	char* dir_name_vec[3]; /*string com o nome das diretorias para onde vão as imagens*/

	dir_name_vec[0] = malloc((strlen(water_dir_s)+1) * sizeof(char));
	dir_name_vec[1] = malloc((strlen(resize_dir_s)+1) * sizeof(char));
	dir_name_vec[2] = malloc((strlen(thumb_dir_s)+1) * sizeof(char));

	sprintf(dir_name_vec[0], "%s", water_dir_s);
	sprintf(dir_name_vec[1], "%s", resize_dir_s);
	sprintf(dir_name_vec[2], "%s", thumb_dir_s);

	for(int i=0; i<3; i++){ /*inicialização das variáveis da estrutura*/
    	image_struct[i]->directory = malloc((strlen(dir_name_vec[i])+1) * 1);
		strcpy(image_struct[i]->directory, dir_name_vec[i]);
		image_struct[i]->watermark_image = watermark_img;
		image_struct[i]->file_name = malloc((biggest_file_name +1) * sizeof(char));
	}

	char *file_in_dir = (char*)malloc((strlen(dir_name) + biggest_file_name +2)* sizeof(char));

	/*criação das threads para as três funções que vão correr todas as imagens*/
    for(int j=0;j<nn_files;j++){
		sprintf(file_in_dir, "%s/%s", dir_name, files[j]);
        for(int i=0; i<3; i++){
            image_struct[i]->image = read_png_file(file_in_dir);
            strcpy(image_struct[i]->file_name, files[j]);
        }   
        pthread_create(&thread_id[0], NULL, watermark_image_function, (void*)image_struct[0]);
        pthread_create(&thread_id[1], NULL, resize_image_function, (void*)image_struct[1]);
        pthread_create(&thread_id[2], NULL, thumb_image_function, (void*)image_struct[2]);  
        for(int i=0; i<3; i++){
            pthread_join(thread_id[i], NULL);
        } 
    }
	gdImageDestroy(watermark_img);

	/*Libertação de memória*/

	for(int i=0; i<3; i++){
		free(image_struct[i]->file_name);
		free(image_struct[i]->directory);
		free(image_struct[i]);
		free(dir_name_vec[i]);
	}
	for(int i=0; i<nn_files; i++){
		free(files[i]);
	}
	free(file_in_dir);
	free(files);
	free(file_name);
	free(watermark_name);
	free(water_dir_s);
	free(thumb_dir_s);
	free(resize_dir_s);

	return 0;	 
}