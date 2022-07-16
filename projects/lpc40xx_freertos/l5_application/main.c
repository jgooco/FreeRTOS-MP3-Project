#include "FreeRTOS.h"
#include "board_io.h"
#include "cli_handlers.h"
#include "common_macros.h"
#include "delay.h"
#include "ff.h"
#include "gpio_isr.h"
#include "mp3_decoder.h"
#include "mp3_lcd.h"
#include "periodic_scheduler.h"
#include "queue.h"
#include "sj2_cli.h"
#include "song_controller.h"
#include "task.h"
#include "uart.h"
#include "uart_lab.h"
#include <stdio.h>

typedef char songdata_t[512];
typedef char songname_t[32];

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;
QueueHandle_t Q_songlist;

TaskHandle_t player_handle;

SemaphoreHandle_t pause_button_pressed;
SemaphoreHandle_t next_button_pressed;
SemaphoreHandle_t volume_up_pressed;
SemaphoreHandle_t volume_down_pressed;

typedef struct mp3_functions {
  gpio_s pause;
  gpio_s next;
  gpio_s volume_up;
  gpio_s volume_down;
} mp3_buttons;

static mp3_buttons button;

volatile int list_of_songs_index = 0;

volatile int song_list_upperbound = 0; // new
volatile int song_list_lowerbound = 2; // new

volatile bool pause = true;

void play_next_isr(void) {
  fprintf(stderr, "I am in next function ISR\n");
  xSemaphoreGiveFromISR(next_button_pressed, NULL);
  LPC_GPIOINT->IO0IntClr = (1 << 29);
}

void pause_button_isr(void) {
  fprintf(stderr, "I am in pause function ISR\n");
  xSemaphoreGiveFromISR(pause_button_pressed, NULL);
  LPC_GPIOINT->IO0IntClr = (1 << 30);
}

void volume_up_button_isr(void) {
  fprintf(stderr, "I am in vol up function ISR\n");
  xSemaphoreGiveFromISR(volume_up_pressed, NULL);
  LPC_GPIOINT->IO0IntClr = (1 << 6);
}

void volume_down_button_isr(void) {
  fprintf(stderr, "I am in vol down function ISR\n");
  xSemaphoreGiveFromISR(volume_down_pressed, NULL);
  LPC_GPIOINT->IO0IntClr = (1 << 1);
}

void mp3_play_next(void *p) {
  while (1) {
    if (xSemaphoreTake(next_button_pressed, portMAX_DELAY)) {
      fprintf(stderr, "I am in next function \n");
      if (list_of_songs_index < song_list__get_item_count()) {
        list_of_songs_index++;

        xQueueSend(Q_songname, song_list__get_name_for_item(list_of_songs_index), 0);
        fprintf(stderr, "Song: %s \n", song_list__get_name_for_item(list_of_songs_index));
        // vTaskDelay(300);

      } else {
        fprintf(stderr, "Gone out of the scope for MP3 Song List \n");
      }
    }
  }
}

void pause_function(void) {
  while (1) {
    if (xSemaphoreTake(pause_button_pressed, portMAX_DELAY)) {
      if (pause) {
        fprintf(stderr, "I am in pause function \n");
        vTaskSuspend(player_handle);
        // vTaskDelay(300);
        pause = false;
      } else {
        vTaskResume(player_handle);
        // vTaskDelay(300);
        pause = true;
      }
    } else {
      fprintf(stderr, "Fail to get semaphore signal from ISR \n");
    }
  }
}

void menu_cursor(void) {
  while (1) {
    if (xSemaphoreTake(pause_button_pressed, portMAX_DELAY)) {
      fprintf(stderr, "I am in menu cursor \n");
      if (list_of_songs_index < song_list__get_item_count() - 1) {
        list_of_songs_index++;
        print_cursor_on_lcd(list_of_songs_index);
      } else {
        list_of_songs_index = 0;
        print_cursor_on_lcd(list_of_songs_index);
      }
      fprintf(stderr, "Song List index: %d \n", list_of_songs_index);
      // xQueueSend(Q_songlist, index, 0);
    }
  }
}

void play_song(void) {
  while (1) {
    if (xSemaphoreTake(next_button_pressed, portMAX_DELAY)) {
      fprintf(stderr, "I am in play song \n");
      // if (xQueueReceive(Q_songlist, index, portMAX_DELAY)) {
      //   fprintf(stderr, "index: %d \n", index);
      // }
      xQueueSend(Q_songname, song_list__get_name_for_item(list_of_songs_index), 0);
    }
  }
}

typedef struct {
  char title[32];
  char artist[32];
} current_song;

void mp3_meta_display(char *song_meta) {
  current_song info = {0};
  int count_name = 0;
  int count_artist = 0;
  for (int i = 0; i < 128; i++) {
    if (i > 2 && i < 33) {
      info.title[count_name++] = (int)(song_meta[i]);
    } else if (i > 32 && i < 63) {
      info.artist[count_artist++] = (int)(song_meta[i]);
    }
  }
  lcd_clear_display();

  uart_write(0xFE);
  uart_write(0x80);
  lcd_write_string("Title: ");
  lcd_write_string(info.title);

  uart_write(0xFE);
  uart_write(0xC0);
  lcd_write_string("Artist: ");
  lcd_write_string(info.artist);
}

void mp3_reader_task(void *p) {
  songname_t songname = {};
  while (1) {
    fprintf(stderr, "Repeat \n");
    if (xQueueReceive(Q_songname, songname, portMAX_DELAY)) {
      printf("Received Song: %s\n", songname);
    }
    fprintf(stderr, "after\n");

    const char *filename = songname;
    FIL file;
    songdata_t buffer = {};
    UINT num_of_bytes_read;
    char song_meta[128];

    FRESULT result = f_open(&file, filename, FA_READ); // open

    if (FR_OK == result) {
      f_lseek(&file, f_size(&file) - (sizeof(char) * 128));
      f_read(&file, song_meta, sizeof(song_meta), &num_of_bytes_read);
      mp3_meta_display(song_meta);
      f_lseek(&file, 0);
      f_close(&file);

      FRESULT result = f_open(&file, filename, FA_READ);

      f_read(&file, buffer, sizeof(buffer), &num_of_bytes_read);
      while (num_of_bytes_read != 0) {
        f_read(&file, buffer, sizeof(buffer), &num_of_bytes_read);
        xQueueSend(Q_songdata, buffer, portMAX_DELAY);
        if (uxQueueMessagesWaiting(Q_songname)) {
          break;
        }
      }

      f_close(&file);
    } else {
      fprintf(stderr, "Error in opening file %s\n", songname);
    }
  }
}

void mp3_player_task(void *p) {
  int counter = 1;
  songdata_t songdata = {};
  while (1) {
    if (xQueueReceive(Q_songdata, &songdata[0], portMAX_DELAY)) {
      for (int i = 0; i < 512; i++) {
        while (!stream_buffer_full) {
          vTaskDelay(1);
        }
        mp3_ssp_write(songdata[i]);
      }
    }
  }
}

uint8_t max_volume = 0x00;
uint8_t min_volume = 0xFA;
uint16_t current_volume = 0x20;

uint8_t max_treble = 0x86;
uint8_t min_treble = 0x96;
uint8_t current_treble = 0x06;

uint8_t max_bass = 0x07;
uint8_t min_bass = 0xF6;
uint8_t current_bass = 0x06;

uint8_t volume_counter = 5;
uint8_t bass_counter = 5;
uint8_t treble_counter = 5;

void volume_up() {
  while (1) {
    if (xSemaphoreTake(volume_up_pressed, portMAX_DELAY)) {
      fprintf(stderr, "In vol up sem.\n");
      if (current_volume <= max_volume) {
        fprintf(stderr, "Volume max.\n");
      } else {
        current_volume = current_volume - 0x10;
        sci_write_reg(0x0B, current_volume, current_volume);
        fprintf(stderr, "Increasing volume\n");
        volume_counter++;
      }
    }
    uart_write(0xFE);
    uart_write(0xD4);
    lcd_write_string("V: ");
    fprintf(stderr, " vol counter %c\n", volume_counter + '0');
    lcd_write_string(volume_counter + '0');
  }
}

void volume_down() {
  while (1) {
    if (xSemaphoreTake(volume_down_pressed, portMAX_DELAY)) {
      fprintf(stderr, "In vol down sem.\n");
      if (current_volume >= min_volume) {
        fprintf(stderr, "Volume min.\n");
      } else {
        current_volume = current_volume + 0x10;
        sci_write_reg(SCI_VOL_ADDR, current_volume, current_volume);
        fprintf(stderr, "Lowering volume.\n");
        volume_counter--;
      }
    }
    uart_write(0xFE);
    uart_write(0xD4);
    lcd_write_string("V: ");
    fprintf(stderr, " vol counter %c\n", volume_counter + '0');
    lcd_write_string(volume_counter + '0');
  }
}

void bass_up() {
  while (1) {
    if (xSemaphoreTake(volume_up_pressed, portMAX_DELAY)) {
      if (current_bass <= max_bass) {
        ;
      } else {
        current_bass = current_bass - 0x20;
        // uint16_t tremble = sci_read_reg(SCI_BASS_ADDR);
        sci_write_reg(SCI_BASS_ADDR, current_bass, current_bass);
        bass_counter++;
      }
    }
  }
}

void bass_down() {
  while (1) {
    if (xSemaphoreTake(volume_down_pressed, portMAX_DELAY)) {
      if (current_bass >= min_bass) {
        ;
      } else {
        current_bass = current_bass + 0x20;
        sci_write_reg(SCI_BASS_ADDR, current_bass, current_bass);
        bass_counter--;
      }
    }
  }
}

void treble_up() {
  while (1) {
    if (xSemaphoreTake(volume_up_pressed, portMAX_DELAY)) {
      if (current_treble >= max_treble) {
        ;
      } else {
        current_treble = current_treble - 0x08;
        sci_write_reg(SCI_BASS_ADDR, current_treble, current_treble);
        treble_counter++;
      }
    }
  }
}

void treble_down() {
  while (1) {
    if (xSemaphoreTake(volume_down_pressed, portMAX_DELAY)) {
      if (current_treble == min_treble) {
        ;
      } else {
        current_treble = current_treble + 0x08;
        sci_write_reg(SCI_BASS_ADDR, current_treble, current_treble);
        treble_counter--;
      }
    }
  }
}

void main(void) {

  pause_button_pressed = xSemaphoreCreateBinary();
  next_button_pressed = xSemaphoreCreateBinary();
  volume_up_pressed = xSemaphoreCreateBinary();
  volume_down_pressed = xSemaphoreCreateBinary();

  button.pause = gpio__construct_as_input(0, 30); // PAUSE Function
  button.next = gpio__construct_as_input(0, 29);  // NEXT Function
  button.volume_up = gpio__construct_as_input(0, 6);
  button.volume_down = gpio__construct_as_input(0, 1);

  mp3_startup();
  // attach_interrupt(30, pause_button_isr); // pause
  // attach_interrupt(29, play_next_isr);    // play next
  // function_callback();

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0__interrupt_dispatcher, "name");
  gpio0__attach_interrupt(30, GPIO_INTR__RISING_EDGE, pause_button_isr);
  gpio0__attach_interrupt(29, GPIO_INTR__RISING_EDGE, play_next_isr);
  gpio0__attach_interrupt(6, GPIO_INTR__RISING_EDGE, volume_up_button_isr);
  gpio0__attach_interrupt(1, GPIO_INTR__RISING_EDGE, volume_down_button_isr);

  song_list__populate();

  uart_init(clock__get_peripheral_clock_hz(), 9600);
  lcd_init();
  // lcd_print_upper_to_lower_songs(song_list_upperbound, song_list_lowerbound); // new

  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Q_songdata = xQueueCreate(1, sizeof(songdata_t));
  Q_songlist = xQueueCreate(1, sizeof(int));

  xTaskCreate(mp3_reader_task, "read task", (512U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_player_task, "play task", (512U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, &player_handle);
  // xTaskCreate(pause_function, "pause", (512U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  // xTaskCreate(mp3_play_next, "play next", (512U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(menu_cursor, "menu cursor", (512U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(play_song, "play song", (512U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  // xTaskCreate(volume_up, "volume up", (512U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  // xTaskCreate(volume_down, "volume down", (512U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(treble_up, "volume up", (512U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(treble_down, "volume down", (512U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);

  sj2_cli__init();
  vTaskStartScheduler();
}

// typedef char songdata_t[512];
// typedef char songname_t[64];

// QueueHandle_t Q_songname;
// QueueHandle_t Q_songdata;

// typedef struct {
//   char title[32];
//   char artist[32];
// } current_song;

// void mp3_reader_task(void *p) {
//   songname_t songname = {};
//   while (1) {
//     xQueueReceive(Q_songname, songname, portMAX_DELAY);
//     const char *filename = songname;
//     mp3_meta_reader(songname);

//     FIL file;
//     songdata_t buffer = {};
//     UINT num_of_bytes_read;
//     FRESULT result = f_open(&file, filename, FA_READ);

//     if (FR_OK == result) {
//       f_read(&file, buffer, sizeof(buffer), &num_of_bytes_read);
//       while (num_of_bytes_read != 0) {
//         f_read(&file, buffer, sizeof(buffer), &num_of_bytes_read);
//         xQueueSend(Q_songdata, buffer, portMAX_DELAY);
//       }
//       f_close(&file);
//     } else {
//       fprintf(stderr, "Error in opening file %s\n", songname);
//     }
//   }
// }

// void mp3_player_task(void *p) {
//   int counter = 1;
//   songdata_t songdata = {};
//   while (1) {
//     xQueueReceive(Q_songdata, &songdata[0], portMAX_DELAY);
//     for (int i = 0; i < sizeof(songdata); i++) {
//       while (!stream_buffer_full) {
//         vTaskDelay(1);
//       }
//       mp3_ssp_write(songdata[i]);
//     }
//   }
// }

// void mp3_meta_reader(char *filename) {
//   char song_meta[128];
//   FIL file;
//   UINT num_of_bytes_read;
//   FRESULT result = f_open(&file, filename, (FA_READ));

//   if (FR_OK == result) {
//     f_lseek(&file, f_size(&file) - (sizeof(char) * 128));
//     f_read(&file, song_meta, sizeof(song_meta), &num_of_bytes_read);
//     mp3_meta_display(song_meta);
//     f_lseek(&file, 0);
//     f_close(&file);
//   } else {
//     fprintf(stderr, "Error in song meta\n");
//   }
// }

// void mp3_meta_display(char *song_meta) {
//   current_song info = {0};
//   int count_name = 0;
//   int count_artist = 0;
//   for (int i = 0; i < 128; i++) {
//     if (i > 2 && i < 33) {
//       info.title[count_name++] = (int)(song_meta[i]);
//     } else if (i > 32 && i < 63) {
//       info.artist[count_artist++] = (int)(song_meta[i]);
//     }
//   }
//   lcd_clear_display();

//   uart_write(0xFE);
//   uart_write(0x80);
//   lcd_write_string("Title: ");
//   lcd_write_string(info.title);

//   uart_write(0xFE);
//   uart_write(0xC0);
//   lcd_write_string("Artist: ");
//   lcd_write_string(info.artist);

//   uart_write(0xFE);
//   uart_write(0xD4);
//   lcd_write_string("VOL: ");
//   lcd_write_string("xx");
// }

// void main(void) {
//   sj2_cli__init();
//   mp3_startup();
//   uart_init(clock__get_peripheral_clock_hz(), 9600);
//   song_list__populate();

//   lcd_init();

//   for (size_t song_number = 0; song_number < song_list__get_item_count(); song_number++) {
//     printf("Song %2d: %s\n", (1 + song_number), song_list__get_name_for_item(song_number));
//   }

//   Q_songname = xQueueCreate(1, sizeof(songname_t));
//   Q_songdata = xQueueCreate(1, sizeof(songdata_t));
//   xTaskCreate(mp3_reader_task, "read task", (2048 * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
//   xTaskCreate(mp3_player_task, "play task", (2048 * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);

//   vTaskStartScheduler();
// }
