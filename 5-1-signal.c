#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>

volatile int hidden_value;
volatile int try_count = 0;
volatile pid_t guesser_id;
volatile int range_start = 1;
volatile int range_end;
volatile int max_rounds = 10;
volatile int round_num = 0;
volatile sig_atomic_t new_guess = 0;
volatile sig_atomic_t last_guess = 0;
volatile sig_atomic_t round_ended = 0;

struct timeval begin_time, finish_time;

void show_timing() {
    gettimeofday(&finish_time, NULL);
    long sec = finish_time.tv_sec - begin_time.tv_sec;
    long usec = finish_time.tv_usec - begin_time.tv_usec;
    double total = sec + usec * 1e-6;
    printf("⌛ Длительность этапа: %.3f сек\n", total);
}

void correct_answer(int sig) {
    printf("\n Участник [%d] нашёл значение %d за %d попыток!\n", 
           getpid(), hidden_value, try_count);
    show_timing();
    round_ended = 1;
    exit(0);
}

void wrong_answer(int sig) {
    printf(" Участник [%d]: Попытка неудачна!\n", getpid());
}

void process_guess(int sig, siginfo_t *info, void *context) {
    last_guess = info->si_value.sival_int;
    new_guess = 1;
}

void submit_attempt(int value) {
    union sigval data;
    data.sival_int = value;
    if (sigqueue(guesser_id, SIGRTMIN, data)) {
        perror("Ошибка передачи данных");
        exit(1);
    }
}

void attempt() {
    if (range_start > range_end || round_ended) {
        return;
    }

    int guess = range_start + (range_end - range_start) / 2;
    try_count++;
    printf("Участник [%d] проверяет значение: %d\n", getpid(), guess);
    submit_attempt(guess);

    if (guess == hidden_value) {
        kill(guesser_id, SIGUSR1);
        round_ended = 1;
    } else {
        if (guess < hidden_value) {
            printf("Участник [%d]: Значение больше!\n", getpid());
            range_start = guess + 1;
        } else {
            printf("Участник [%d]: Значение меньше!\n", getpid());
            range_end = guess - 1;
        }
        kill(guesser_id, SIGUSR2);
    }
}

void guesser_role() {
    printf("\n Участник [%d] начал поиск числа\n", getpid());
    
    signal(SIGUSR1, correct_answer);
    signal(SIGUSR2, wrong_answer);
    
    struct sigaction cfg;
    cfg.sa_flags = SA_SIGINFO;
    cfg.sa_sigaction = process_guess;
    sigemptyset(&cfg.sa_mask);
    sigaction(SIGRTMIN, &cfg, NULL);

    while (!round_ended) {
        pause();
        if (new_guess) {
            new_guess = 0;
        }
    }
    exit(0);
}

void hider_role(int limit) {
    hidden_value = rand() % limit + 1;
    try_count = 0;
    range_start = 1;
    range_end = limit;
    round_ended = 0;
    
    printf("\n=== Этап %d ===\n", round_num);
    printf(" Участник [%d] загадал число 1-%d\n", getpid(), limit);
    gettimeofday(&begin_time, NULL);
    
    signal(SIGALRM, attempt);
    
    while (!round_ended && try_count < 10) {
        alarm(1);
        pause();
    }

    if (!round_ended) {
        printf("\n Участник [%d] не определил число %d за 10 попыток!\n", 
               guesser_id, hidden_value);
        show_timing();
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Формат: %s <макс_число>\n", argv[0]);
        return 1;
    }

    int limit = atoi(argv[1]);
    range_end = limit;
    srand(time(NULL));

    printf(" Контроллер [%d] инициировал игру\n", getpid());

    while (round_num < max_rounds) {
        round_num++;
        
        pid_t guesser = fork();
        if (guesser < 0) {
            perror("Ошибка создания процесса");
            exit(1);
        }

        if (guesser == 0) {
            guesser_role();
        } else {
            guesser_id = guesser;
            hider_role(limit);
            
            kill(guesser, SIGTERM);
            wait(NULL);
            
            pid_t hider = fork();
            if (hider < 0) {
                perror("Ошибка создания процесса");
                exit(1);
            }

            if (hider == 0) {
                guesser_role();
            } else {
                guesser_id = hider;
                hider_role(limit);
                
                kill(hider, SIGTERM);
                wait(NULL);
            }
        }
    }

    printf("\n Контроллер [%d] завершил игру. Проведено этапов: %d.\n", 
           getpid(), max_rounds);
    return 0;
}
