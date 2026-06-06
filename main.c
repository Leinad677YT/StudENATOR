#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define loop() for(;;)

#define LEINAD_EXPECTED_HEADER_single "header.leinad.dontchange tester/1.0\n"
#define LEINAD_EXPECTED_HEADER_wrapper "header.leinad.dontchange tester_wrapper/1.0\n"


#define LEINAD_MAX_ANTE_LENGTH 120
#define LEINAD_MAX_CONS_LENGTH 120
#define LEINAD_MAX_FILE_LENGTH 100
// should be self explanatory

#define LEINAD_MAX_LINE_LENGTH (1+LEINAD_MAX_ANTE_LENGTH+LEINAD_MAX_CONS_LENGTH)
// buffers should hold LENGTH+1 for the final '\0'
// if not enough, make it 219304 :works_as_intended:


// holds data for a single tester
struct pair_db {
    struct pairs {
        char corresponds_to[LEINAD_MAX_CONS_LENGTH];
        char question_about[LEINAD_MAX_ANTE_LENGTH];
    } *pairs;
    short num_options;
    int num_pairs;
};

// holds data for all the loaded testers
struct data {
    int num_db;
    struct pair_db** pair_db;
};

static char answer_letters[9] = {
    'a','b','c','d','e','f','g','h','i'
};



/**
 * read up to LEINAD_MAX_LINE_LENGTH chars from the FD `io` into `output`
 * if read line ended properly, the last read character will be `\n`, otherwise unspecified
 * @return amount of chars read or -1 on error
 */
static int leinad_io_readline(int io, char output[LEINAD_MAX_LINE_LENGTH]);

struct pair_db* leinad_tester_single_readfile(int data_fd);

struct data* leinad_tester_wrapper_readfile(int fd);



int main(void){

    int read_chars;
    char file[LEINAD_MAX_FILE_LENGTH];
    char buffer[LEINAD_MAX_LINE_LENGTH+1];
    
    struct data* data = NULL;

    int data_fd = -1;

change_file:
    if (data_fd != -1) close(data_fd); data_fd = -1;
    if (data != NULL && data->pair_db != NULL) for(int _i = 0; _i < data->num_db; _i++) {if (data->pair_db[_i] != NULL) free(data->pair_db[_i]);}
    if (data != NULL && data->pair_db != NULL) free(data->pair_db);
    if (data != NULL) free(data); data = NULL;
    loop() {
        printf("Please introduce the path to the data file or `Q` to exit:\n > ");
        scanf("%s",file);

        if (!strcmp(file,"Q")) goto end;

        char aux;
        while ((aux = getchar()) != '\n' && aux != EOF);


        data_fd = open(file,O_RDONLY);
        
        if (data_fd == -1) {
            perror("Could not access the specified data file: ");
        }
        else break;
    }

  { // read header
    memset(buffer,0,sizeof(char[LEINAD_MAX_LINE_LENGTH+1]));
    read_chars = leinad_io_readline(data_fd, buffer);

    if (read_chars ==35 || !strcmp(buffer,LEINAD_EXPECTED_HEADER_single)) {
        data = malloc(sizeof(struct data));
        data->pair_db = malloc(sizeof(struct pair_db));

        data->pair_db[0] = leinad_tester_single_readfile(data_fd);
        if (data->pair_db == NULL) goto change_file;

        data->num_db = 1;
        printf("file_format: single\n");
    }
    else if (read_chars ==43 || !strcmp(buffer,LEINAD_EXPECTED_HEADER_wrapper)){
        data = leinad_tester_wrapper_readfile(data_fd);
        if (data == NULL) {
            goto change_file;
        }
        printf("file_format: wrapper\n");
    }
    else {
        printf("Specified file does not contain data for the tester on 1.0!\n");
        goto change_file;
    }
  }
    
  printf(
        """"""
        "\n"
        "finished_data_loading\n"
        "Opened test databases: %d\n"
        "\n"
        """""",
        data->num_db
    );

    fflush(stdout);


  { // main loop

    int questions_asked = 0;
    int questions_correct = 0;
    int selected_pair = 0;
    int all_pairs[9] = {0};

    srand(time(NULL));
    loop(){
        
        int selected_db = rand() % data->num_db;


        selected_pair = rand() % data->pair_db[selected_db]->num_options;

        for(int i = 0; i < data->pair_db[selected_db]->num_options; i++){
            loop() {
                int success = 1;
                all_pairs[i] = rand() % data->pair_db[selected_db]->num_pairs;

                for(int j = 0; j < i && success; j++){
                    if (all_pairs[i] == all_pairs[j]) success = 0;
                }

                if (success) break;
            }
        }


        // print question
        questions_asked++;
        printf(
            """"""
            "%d - %s\n"
            """""",
            questions_asked,
            data->pair_db[selected_db]->pairs[all_pairs[selected_pair]].question_about
        );
        for(int i = 0; i < data->pair_db[selected_db]->num_options; i++){
            printf(
                """"""
                "%c) %s\n"
                """""",
                answer_letters[i],
                data->pair_db[selected_db]->pairs[all_pairs[i]].corresponds_to
            );
        }

        printf("\n");
        // read answer
        char answer;
        printf(
            """"""
            "Introduce the option (`%c` - `%c`) to answer\n"
            "Introduce `!` to go back to file selection\n"
            "Introduce `Q` to exit\n\n"
            " > "
            """""",
            answer_letters[0],
            answer_letters[data->pair_db[selected_db]->num_options -1]
        );

        answer = getchar();
        int correct = answer == answer_letters[selected_pair];

        if (answer == '!') correct = -1;
        if (answer == 'Q') correct = -2;

        // clear buffer
        while ((answer = getchar()) != '\n' && answer != EOF);
 
        if (correct == -1) goto change_file;
        if (correct == -2) goto end;

        if (correct) {
            questions_correct++;
            printf(
                """"""
                "Correct answer! :tada:\n"
                "correctly answered: %.2f%%\n"
                "\n\n\n"
                """""",
                100*(float)questions_correct / (float)questions_asked
            );
        }
        else {
            printf(
                """"""
                "Wrong answer! :( the correct one was `%c`: %s\n"
                "correctly answered: %.2f%%\n"
                "\n\n\n"
                """""",
                answer_letters[selected_pair], data->pair_db[selected_db]->pairs[all_pairs[selected_pair]].corresponds_to,
                100*(float)questions_correct / (float)questions_asked
            );
        }

    }

end:
    if (data_fd != -1) close(data_fd);
    if (data != NULL && data->pair_db != NULL) for(int _i = 0; _i < data->num_db; _i++) {if (data->pair_db[_i] != NULL) free(data->pair_db[_i]);}
    if (data != NULL && data->pair_db != NULL) free(data->pair_db);
    if (data != NULL) free(data);
    return EXIT_SUCCESS;
  }
}




static int leinad_io_readline(int io, char output[LEINAD_MAX_LINE_LENGTH]){
    
    if (io == -1) return -1;

    long long int offset;
    size_t read_chars;
    int i;
    char line[LEINAD_MAX_LINE_LENGTH+1] = {0};

    // GET CURRENT OFFSET
    offset = lseek(io,0,SEEK_CUR);

    read_chars = read(io, line, LEINAD_MAX_LINE_LENGTH);


    for (i = 0;i < read_chars; i++) {
        output[i] = line[i];
        if (line[i] == '\0' || line[i] == '\n') break;
    }

    // remove the line IF FOUND from the IO offset
    lseek(io, offset+i+ (line[i] == '\n' ? 1 : 0), SEEK_SET);

    return i;
}

struct pair_db* leinad_tester_single_readfile(int data_fd) {
    struct pair_db* pair_db = malloc(sizeof(struct pair_db));
    
    char buffer[LEINAD_MAX_LINE_LENGTH+1];
    int read_chars;

  { // read tester-specific header args

    // options
    memset(buffer,0,sizeof(char[LEINAD_MAX_LINE_LENGTH+1]));
    read_chars = leinad_io_readline(data_fd, buffer);
    
    if (read_chars !=1 || !(buffer[0] - '0')) {
        printf(
            """"""
            "Specified file does not contain a valid options number!\n"
            "- read: %s"
            "It should be an integer inside the range [1,9]\n"
            """""",
            buffer
        );
        return NULL;
    }
    else pair_db->num_options = buffer[0] - '0';

    
    // pairs
    memset(buffer,0,sizeof(char[LEINAD_MAX_LINE_LENGTH+1]));
    read_chars = leinad_io_readline(data_fd, buffer);

    if (
        read_chars ==0 || 
        (pair_db->num_pairs = strtol(buffer,NULL,10)) < pair_db->num_options
    ) {
        printf(
            """"""
            "Specified file does not contain a valid pair amount!\n"
            "It should be at least as high as the options amount\n"
            """"""
        );
        return NULL;
    }

    leinad_io_readline(data_fd,buffer);
  }

  { // read pairs
    pair_db->pairs = malloc(sizeof(struct pairs) * pair_db->num_pairs);
    memset(pair_db->pairs,0,sizeof(struct pairs) * pair_db->num_pairs);

    for(int i = 0; i<pair_db->num_pairs; i++) {
        int c;
        char buffer[LEINAD_MAX_LINE_LENGTH+1] = {0};

        read_chars = leinad_io_readline(data_fd, buffer);

        for(c = 0; c < read_chars; c++){
            if (buffer[c] =='\0' || buffer[c] == '\n' || buffer[c] == ';') break;

            pair_db->pairs[i].corresponds_to[c] = buffer[c];
        }

        for(int _c = 0; _c + c < read_chars; _c++) {
            if (buffer[_c + c +1] =='\0' || buffer[_c + c +1] == '\n') break;

            pair_db->pairs[i].question_about[_c] = buffer[_c +1 + c];
        }

    }
  }

    return pair_db;
}


struct data* leinad_tester_wrapper_readfile(int data_fd) {
    struct data* data;
    data = malloc(sizeof(struct data));

    char buffer[LEINAD_MAX_LINE_LENGTH+1];
    int read_chars;

  { // read tester-specific header args

    // files
    memset(buffer,0,sizeof(char[LEINAD_MAX_LINE_LENGTH+1]));
    read_chars = leinad_io_readline(data_fd, buffer);

    if (
        read_chars ==0 || 
        (data->num_db = strtol(buffer,NULL,10)) <= 0
    ) {
        printf(
            """"""
            "Specified file does not contain a valid file amount!\n"
            "It should be at least 1\n"
            """"""
        );
        return NULL;
    }

    leinad_io_readline(data_fd,buffer);
  }

  { // read all files
    data->pair_db = malloc(sizeof(struct pair_db*) * data->num_db);

    int single_fd[data->num_db];
    int i;

    int failed = 0;

    char buffer[LEINAD_MAX_LINE_LENGTH+1];
    for(i = 0; i<data->num_db; i++) {
        char buffer_2[LEINAD_MAX_LINE_LENGTH+1] = {0};
        memset(buffer,0,sizeof(char) * (LEINAD_MAX_LINE_LENGTH+1));
        read_chars = leinad_io_readline(data_fd, buffer);

        buffer[read_chars] = '\0';
        single_fd[i] = open(buffer,O_RDONLY);

        if (single_fd[i] == -1) {
            failed = 1;
            break;
        }

        read_chars = leinad_io_readline(single_fd[i], buffer_2);
        if (read_chars !=35 || strcmp(buffer_2,LEINAD_EXPECTED_HEADER_single)) {
            printf("Specified wrapped file is not a valid wrappable file for the tester on 1.0!\n");
        }

        data->pair_db[i] = leinad_tester_single_readfile(single_fd[i]);
        if (data->pair_db[i] == NULL) {
            failed = 1;
            break;
        }
    }

    for(int j = 0; j < i; j++) {
        close(single_fd[j]);
    }

    if (failed) {
        printf(
            """"""
            "The specified file inside the wrapper cannot be accessed\n"
            "# %s\n"
            """""",
            buffer
        );

        for(int j = 0; j < i; j++) {
            close(single_fd[j]);
            free(data->pair_db[j]->pairs);
        }
        free(data->pair_db);
        free(data);
        return NULL;
    }

  }

    return data;
}
