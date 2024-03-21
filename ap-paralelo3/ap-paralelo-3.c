#include "gd.h"
#include "image-lib.h"
#include <pthread.h>
#include <string.h>
#include <unistd.h>

/* the directories wher output files will be placed */
#define RESIZE_DIR "/serial-Resize/"
#define THUMB_DIR "/serial-Thumbnail/"
#define WATER_DIR "/serial-Watermark/"

/*estrutura com a informação da imagem*/
typedef struct Image_t
{
    gdImagePtr watermark_image;
    char* dir_vec[3];
    char* directory;
    char** file_name;
    int n_files;
}Image;

/*pipe*/
int fd[3][2];

int n_thread;
pthread_mutex_t lock;

int counter=0;

/******************************************************************************
 * watermark_thread_function (void *image_struct)
 *
 * Arguments: 
 *    void *image_struct (contém o ponteiro para a struct image_struct)
 * 
 * Returns: (void*)info
 *
 * Description: Cria uma thumbnail da imagem original
 *****************************************************************************/
void* watermark_thread_func(void* image_struct)
{
    Image *info=(Image*)image_struct;
    int index=0;

    char out_file_name[100], file_in_dir[100];

    while(1){
        /*leitura do indíce da próxima imagem a processar*/
        read(fd[0][0], &index, sizeof(int));
        if(index==-1){ /*se o indice for -1 isso quer dizer que todas as imagens foram processadas*/
            break;
        }

        gdImagePtr image_ptr;

        sprintf(file_in_dir, "%s/%s", info->directory, info->file_name[index]); /*nome da imagem original na diretoria*/
        sprintf(out_file_name, "%s%s", info->dir_vec[2], info->file_name[index]);/*local onde queremos guardar a imagem*/

        if(access(out_file_name, F_OK) == -1){ /*Se a imagem ainda não existir cria*/
            gdImagePtr out_watermark_img;

            image_ptr = read_png_file(file_in_dir); /*leitura da imagem original para a variavel image_ptr*/
            if(image_ptr == NULL){ /*se o processo de leitura falhar*/
                fprintf(stderr, "Impossible to import image: %s\n", info->file_name[index]);
                write(fd[1][1], &index, sizeof(int)); /*escrever no pipe seguinte a imagem*/
                pthread_mutex_lock(&lock);
                counter++;
                pthread_mutex_unlock(&lock);
                continue;
            }

            out_watermark_img = add_watermark(image_ptr, info->watermark_image); /*Aplicar watermark na imagem*/
            if (out_watermark_img == NULL) {
                fprintf(stderr, "Impossible to create thumbnail of %s image\n", info->file_name[index]);
            }
            else{
                printf("watermark  %s\n", info->file_name[index]);
                /* save watermark */
                if(write_png_file(out_watermark_img, out_file_name) == 0){ 
                    fprintf(stderr, "Impossible to write %s image\n", out_file_name);
                }
            }
            /*libertação dos ponteiros das imagens*/
            gdImageDestroy(out_watermark_img);
            gdImageDestroy(image_ptr); 
        }
        else{ /*Imagem ja foi criada logo passa à frente*/
		    printf("Imagem %s ja está presente na diretoria serial-watermark\n", info->file_name[index]);
	    }	

        /*escrita no pipe da função seguinte*/
        write(fd[1][1], &index, sizeof(int));

        pthread_mutex_lock(&lock); /*os mutexes são usados para evitar erros na incrementação pois esta linha é uma região crítica*/
        counter++;
        pthread_mutex_unlock(&lock);

    }
    pthread_exit((void*)info);
}

/******************************************************************************
 * resize_thread_function (void *image_struct)
 *
 * Arguments: 
 *    void *image_struct (contém o ponteiro para a struct image_struct)
 * 
 * Returns: (void*)info
 *
 * Description: Cria uma thumbnail da imagem original
 *****************************************************************************/
void* resize_thread_func(void *image_struct)
{
    Image *info=(Image*)image_struct;
    int index=0;
    char out_file_name[100], file_in_dir[100];
    gdImagePtr image_ptr, out_resized_img;

    while(1){
        /*leitura do indíce da próxima imagem a processar*/
        read(fd[1][0], &index, sizeof(int));
        if(index==-1){ /*se o indice for -1 isso quer dizer que todas as imagens foram processadas*/
            break;
        }

        sprintf(file_in_dir, "%s%s", info->dir_vec[2], info->file_name[index]); /*nome da imagem com a watermark aplicada*/
        sprintf(out_file_name, "%s%s", info->dir_vec[0], info->file_name[index]);/*local onde queremos guardar a imagem*/
        
        if(access(out_file_name, F_OK) == -1){ /*Se a imagem ainda não existir cria*/
            image_ptr = read_png_file(file_in_dir);/*leitura da imagem com watermark para o ponteiro*/
            if(image_ptr == NULL){ /*se o processo de leitura falhar*/
                fprintf(stderr, "Impossible to import image: %s\n", info->file_name[index]);
                write(fd[2][1], &index, sizeof(int)); /*escrever no pipe seguinte o indice da imagem*/
                pthread_mutex_lock(&lock);
                counter++;
                pthread_mutex_unlock(&lock);
                continue;
            }

            out_resized_img = resize_image(image_ptr, 100); /* Altera o tamanho da imagem*/
            if (out_resized_img == NULL) {
                fprintf(stderr, "Impossible to resize %s image\n", info->file_name[index]);
            }
            else{
                printf("resize  %s\n", info->file_name[index]);
                /* save resized */
                if(write_png_file(out_resized_img, out_file_name) == 0){ 
                    fprintf(stderr, "Impossible to write %s image\n", out_file_name);
                }
                gdImageDestroy(out_resized_img);
            }
            gdImageDestroy(image_ptr);
	    }
	    else{ /*Imagem ja foi criada logo passa à frente*/
		    printf("Imagem %s ja está presente na diretoria serial-resize\n", info->file_name[index]);
	    }	

        /*escrita no pipe da função seguinte*/
        write(fd[2][1], &index, sizeof(int));

        pthread_mutex_lock(&lock); /*os mutexes são usados para evitar erros na incrementação pois esta linha é uma região crítica*/
        counter++;
        pthread_mutex_unlock(&lock);

    }
    pthread_exit((void*)info);
}

/******************************************************************************
 * thumb_thread_function (void *image_struct)
 *
 * Arguments: 
 *    void *image_struct (contém o ponteiro para a struct image_struct)
 * 
 * Returns: (void*)info
 *
 * Description: Cria uma thumbnail da imagem original
 *****************************************************************************/
void* thumb_thread_func(void *image_struct)
{
    Image *info=(Image*)image_struct;
    int index=0, stopper=-1;
    char out_file_name[100], file_in_dir[100];

    while(1){
        /*leitura do indíce da imagem para o pipe*/
        read(fd[2][0], &index, sizeof(index));
        if(index==-1){ /*se o indice for -1 então podemos parar o pipeline*/
            break;
        }

        gdImagePtr image_ptr;

        sprintf(file_in_dir, "%s%s", info->dir_vec[2], info->file_name[index]); /*nome da imagem com a watermark aplicada*/
        sprintf(out_file_name, "%s%s", info->dir_vec[1], info->file_name[index]);
        
        if(access(out_file_name, F_OK) == -1){ /*Se a imagem ainda não existir cria*/
            gdImagePtr out_thumb_img;

            image_ptr = read_png_file(file_in_dir);
            if(image_ptr == NULL){ /* se o processo de leitura falhar */
                fprintf(stderr, "Impossible to import image: %s\n", info->file_name[index]);
                pthread_mutex_lock(&lock);
                counter++; /* aumentamos o contador mas não escrevemos no pipe porque estamos na ultima função */
                pthread_mutex_unlock(&lock);
                continue;
            }

            out_thumb_img = resize_image(image_ptr, 100); /* Altera o tamanho da imagem*/
            if (out_thumb_img == NULL) {
                fprintf(stderr, "Impossible to thumb %s image\n", info->file_name[index]);
            }
            else{
                printf("thumb  %s\n", info->file_name[index]);
                /* save resized */
                if(write_png_file(out_thumb_img, out_file_name) == 0){ 
                    fprintf(stderr, "Impossible to write %s image\n", out_file_name);
                }
                gdImageDestroy(out_thumb_img);
            }
            gdImageDestroy(image_ptr);
	    }
	    else{ /*Imagem ja foi criada logo passa à frente*/
		    printf("Imagem %s ja está presente na diretoria serial-thumb\n", info->file_name[index]);
	    }	

        pthread_mutex_lock(&lock);/*os mutexes são usados para evitar erros na incrementação pois esta linha é uma região crítica*/
        counter++;
        pthread_mutex_unlock(&lock);

        /*quando todas as imagens foram processadas vamos enviar -1 para todos os pipes para parar o pipeline*/
        if(counter>=(info->n_files)*3){
            for(int i=0; i<n_thread; i++){
                write(fd[1][1], &stopper, sizeof(int));
                write(fd[2][1], &stopper, sizeof(int));
                write(fd[0][1], &stopper, sizeof(int));
            }
        }
        
    }
    pthread_exit((void*)info);
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
    char* file_name, *dir_name=argv[1];
    int nn_files=0, biggest_file_name=0;

    /*numero de threads por estágio*/
    n_thread=atoi(argv[2]);

    /*nome do ficheiro de texto com o nome das imagens*/
    file_name=malloc((strlen(dir_name) + 16)*sizeof(char));
    sprintf(file_name, "%s/image-list.txt", dir_name);

    /*inicialização do pipe*/
    for(int i=0; i<3; i++){
        if(pipe(fd[i])!=0){
            return 1;
        }
    }

    /*ponteiro para o ficheiro com o nome das imagens*/
    FILE *fp=fopen(file_name, "r");
    if(fp==NULL){
        printf("Couldnt open file");
		exit(-1);
    }

    /*vetor com o nome das imagens a processar*/
	char **files;

    /*String com o nome do ficheiro onde a imagem é guardada*/
	char buffer[100];

	while(fscanf(fp, "%s", buffer)==1){ /*Primeira leitura do ficheiro para obter o número de imagens*/
		nn_files++;
	}

    files = (char**)malloc(nn_files*sizeof(char*));

	fseek(fp, 0, SEEK_SET); /*colocar o ponteiro do ficheiro no inicio*/

	nn_files=0;
    int wordlen=0;
	while(fscanf(fp, "%s", buffer)==1){ /*Ler cada palavra do ficheiro e guardá-la no vetor files*/
		wordlen=strlen(buffer);
		if(wordlen>biggest_file_name){ /*Descobrir a maior palavra do ficheiro*/
			biggest_file_name = wordlen;
		}
		files[nn_files] = (char*)malloc((wordlen+1) * sizeof(char));
		strcpy(files[nn_files] , buffer);
		nn_files++;
	}

	fclose(fp);

    /*id das threads*/
    pthread_t thread_id[3*n_thread];

    /*estrutura com a informação das imagens*/ 
    Image *image_struct=malloc(1 * sizeof(Image));

    gdImagePtr watermark_img;
    /*nome da imagem de watermark*/
    char* watermark_name = malloc((strlen(dir_name)+15)*sizeof(char));
    sprintf(watermark_name, "%s/watermark.png", dir_name);

    /*Criar o pointer para a imagem que vai ser a watermark*/
    watermark_img = read_png_file(watermark_name);
	if(watermark_img == NULL){
		fprintf(stderr, "Impossible to read %s image\n", "watermark.png");
		exit(-1);
	}

    /*vetor com o nome das diretorias onde vamos guardar as imagens*/
    char* dir_name_vec[3];
	int dir_name_len = strlen(dir_name);
	dir_name_vec[0] = malloc((dir_name_len + 17) * sizeof(char));
	dir_name_vec[1] = malloc((dir_name_len + 20) * sizeof(char));
	dir_name_vec[2] = malloc((dir_name_len + 20) * sizeof(char));

	/*Criação das strings do nome das diretorias*/
	sprintf(dir_name_vec[0], "%s%s", dir_name, RESIZE_DIR);
	sprintf(dir_name_vec[1], "%s%s", dir_name, THUMB_DIR);
	sprintf(dir_name_vec[2], "%s%s", dir_name, WATER_DIR);

    /*Criação das diretorias de output para guardar as imagens*/
	if (create_directory(dir_name_vec[0]) == 0){
		fprintf(stderr, "Impossible to create %s directory\n", dir_name_vec[0]);
		exit(-1);
	}
	if (create_directory(dir_name_vec[1]) == 0){
		fprintf(stderr, "Impossible to create %s directory\n", dir_name_vec[1]);
		exit(-1);
	}
	if (create_directory(dir_name_vec[2]) == 0){
		fprintf(stderr, "Impossible to create %s directory\n", dir_name_vec[2]);
		exit(-1);
	}

    /*inicialização das variaveis da estrutura*/
    image_struct->file_name = (char**)malloc(nn_files * sizeof(char*));
    for(int i=0; i<nn_files; i++){
        image_struct->file_name[i] = malloc((strlen(files[i])+1) * sizeof(char));
        sprintf(image_struct->file_name[i], "%s", files[i]); 
    }
    image_struct->dir_vec[0] = malloc((dir_name_len + strlen(RESIZE_DIR)+1) * sizeof(char));    
    image_struct->dir_vec[1] = malloc((dir_name_len + strlen(THUMB_DIR)+1) * sizeof(char));  
    image_struct->dir_vec[2] = malloc((dir_name_len + strlen(WATER_DIR)+1) * sizeof(char));

    image_struct->directory = malloc(strlen(dir_name)+1*sizeof(char));

    strcpy(image_struct->dir_vec[0], dir_name_vec[0]);  
    strcpy(image_struct->dir_vec[1], dir_name_vec[1]);
    strcpy(image_struct->dir_vec[2], dir_name_vec[2]);

    image_struct->watermark_image = watermark_img;  
    strcpy(image_struct->directory, dir_name);

    image_struct->n_files=nn_files;

    /*inicialização do mutex*/
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    /*abertura das thread_fucntions*/
    for(int i=0; i<n_thread*3; i+=3){
        pthread_create(&thread_id[i], NULL, watermark_thread_func, (void*)image_struct);
        pthread_create(&thread_id[i+1], NULL, thumb_thread_func, (void*)image_struct);
        pthread_create(&thread_id[i+2], NULL, resize_thread_func, (void*)image_struct);
    }

    int index=0;
    /*escrita dos indices das imagens na pipeline*/
    for(int i=0; i<nn_files; i++){
        write(fd[0][1], &index, sizeof(int));
        index++;
    }

    for(int i=0; i<n_thread*3; i++){
        pthread_join(thread_id[i], NULL);
    }

    /*libertação de memória*/
    for(int i=0; i<3; i++){
        free(image_struct->dir_vec[i]);
    }

    for(int j=0; j<nn_files; j++){
        free(files[j]);
    }
    free(image_struct->file_name);
    free(image_struct->directory);
    free(files);

    for(int i=0; i<3; i++){
        free(dir_name_vec[i]);
    }
    free(watermark_name);
    free(file_name);
    gdImageDestroy(watermark_img);
    pthread_mutex_destroy(&lock);

    return 0;
}