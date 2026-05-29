#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "processor.h"
#include "parser.h"
#include "pipeline.h"
#include "printer.h"

/* ============================================================
 * Simple console UI helpers
 * ============================================================ */

static void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void pause_and_continue(void) {
    printf("\nPress ENTER to continue...");
    fflush(stdout);
    /* flush any leftover newline first */
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void print_header(void) {
    printf("=========================================\n");
    printf("     PROCESSOR PIPELINE SIMULATOR        \n");
    printf("  5-Stage: IF(2) ID(2) EX(2) MEM(1) WB(1)\n");
    printf("=========================================\n\n");
}

static void print_menu(void) {
    printf("  1) Run simulation (uses program.txt)\n");
    printf("  2) Show program.txt contents\n");
    printf("  0) Exit\n\n");
    printf("Choice: ");
    fflush(stdout);
}

/* Read and display the current program.txt */
static void show_program_file(void) {
    clear_screen();
    print_header();
    printf("--- Contents of program.txt ---\n\n");

    FILE *f = fopen("program.txt", "r");
    if (!f) {
        printf("  (file not found)\n");
    } else {
        char line[128];
        int n = 0;
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = '\0';
            if (strlen(line) > 0)
                printf("  [%2d]  %s\n", n++, line);
        }
        fclose(f);
        if (n == 0) printf("  (file is empty)\n");
    }
    pause_and_continue();
}

/* Run the full simulation */
static void run_simulation(void) {
    clear_screen();
    print_header();

    Processor cpu;
    init_processor(&cpu);

    int total = parseAndLoadFile(cpu.memory, 1024, "program.txt");
    if (total <= 0) {
        printf("\n[ERROR] Could not load program.txt\n");
        printf("Make sure program.txt exists in the same folder as this program.\n");
        pause_and_continue();
        return;
    }

    printf("Loaded %d instruction(s).\n", total);

    runPipeline(&cpu, total);

    print_all_registers(&cpu);
    print_data_memory_nonzero(&cpu);

    printf("\nShow full memory dump? (y/n): ");
    fflush(stdout);
    char buf[8];
    if (fgets(buf, sizeof(buf), stdin) && (buf[0] == 'y' || buf[0] == 'Y')) {
        print_instruction_memory(&cpu, total);
        print_data_memory_full(&cpu);
    }

    pause_and_continue();
}

/* ============================================================
 * Main
 * ============================================================ */
int main(void) {
    char input[8];

    while (1) {
        clear_screen();
        print_header();
        print_menu();

        if (!fgets(input, sizeof(input), stdin)) break;
        int choice = atoi(input);

        switch (choice) {
            case 1: run_simulation();    break;
            case 2: show_program_file(); break;
            case 0:
                clear_screen();
                printf("Goodbye.\n\n");
                return 0;
            default:
                printf("  Invalid choice.\n");
                pause_and_continue();
        }
    }

    return 0;
}