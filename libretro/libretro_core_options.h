#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include "../libretro-common/include/libretro.h"
#include "../libretro-common/include/retro_inline.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_ENGLISH */

/* Default language:
 * - All other languages must include the same keys and values
 * - Will be used as a fallback in the event that frontend language
 *   is not available
 * - Will be used as a fallback for any missing entries in
 *   frontend language definition */

struct retro_core_option_definition option_defs_us[] = {
   {
      "vitaquakeii_framerate",
      "Framerate (restart)",
      "Modify framerate. Requires a restart.",
      {
         { "auto",            "Auto"},
         { "50",              "50fps"},
         { "60",              "60fps"},
         { "72",              "72fps"},
         { "75",              "75fps"},
         { "90",              "90fps"},
         { "100",              "100fps"},
         { "119",              "119fps"},
         { "120",              "120fps"},
         { "144",              "144fps"},
         { "155",              "155fps"},
         { "160",              "160fps"},
         { "165",              "165fps"},
         { "180",              "180fps"},
         { "200",              "200fps"},
         { "240",              "240fps"},
         { "244",              "244fps"},
         { NULL, NULL },
      },
      "auto"
   },
   {
      "vitaquakeii_resolution",
      "Internal resolution (restart)",
      "Configure the resolution. Requires a restart.",
      {
         { "320x240",   NULL },
         { "480x272",   NULL },
         { "640x368",   NULL },
         { "640x480",   NULL },
         { "720x408",   NULL },
         { "960x544",   NULL },
         { "960x720",   NULL },
         { "1280x720",   NULL },
         { "1280x960",   NULL },
         { "1600x1200",   NULL },
         { "1920x1080",   NULL },
         { "1920x1440",   NULL },
         { "2240x1680",   NULL },
         { "2560x1440",   NULL },
         { "2560x1920",   NULL },
         { "2880x2160",   NULL },
         { "3200x2400",   NULL },
         { "3520x2640",   NULL },
         { "3840x2160",   NULL },
         { "3840x2880",   NULL },
         { "4160x3120",   NULL },
         { "4480x3360",   NULL },
         { "4800x3600",   NULL },
         { "5120x2880",   NULL },
         { "5120x3840",   NULL },
         { "5440x4080",   NULL },
         { "5760x4320",   NULL },
         { "6080x4560",   NULL },
         { "6400x4800",   NULL },
         { "6720x5040",   NULL },
         { "7040x5280",   NULL },
         { "7360x5520",   NULL },
         { "7680x4320",   NULL },
         { "7680x5760",   NULL },
         { "8000x6000",   NULL },
         { "8320x6240",   NULL },
         { "8640x6480",   NULL },
         { "8960x6720",   NULL },
         { "9280x6960",   NULL },
         { "9600x7200",   NULL },
         { "9920x7440",   NULL },
         { "10240x7680",   NULL },
         { "10560x7920",   NULL },
         { "10880x8160",   NULL },
         { "11200x8400",   NULL },
         { "11520x8640",   NULL },
         { "11840x8880",   NULL },
         { "12160x9120",   NULL },
         { "12480x9360",   NULL },
         { "12800x9600",   NULL },
         { "13120x9840",   NULL },
         { "13440x10080",   NULL },
         { "13760x10320",   NULL },
         { "14080x10560",   NULL },
         { "14400x10800",   NULL },
         { "14720x11040",   NULL },
         { "15040x11280",   NULL },
         { "15360x8640",   NULL },
         { "15360x11520",   NULL },
         { NULL, NULL },
      },
      "960x544"
   },
   {
      "vitaquakeii_dithered_filtering",
      "Dithered filtering",
      "Enables kernel-based software texture smoothing/filtering. Will take a hit on performance. Only works with the software renderer.",
      {
         { "disabled",  "Disabled" },
         { "enabled",   "Enabled" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "vitaquakeii_shadows",
      "Dynamic Shadows",
      "Enables dynamic shadows rendering. Only works with the hardware renderer.",
      {
         { "disabled",  "Disabled" },
         { "enabled",   "Enabled" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "vitaquakeii_rumble",
      "Rumble",
      "Enables joypad rumble.",
      {
         { "disabled",  "Disabled" },
         { "enabled",   "Enabled" },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "vitaquakeii_specular",
      "Specular Mode",
      "Makes every level be specular. Only works with the hardware renderer.",
      {
         { "disabled",  "Disabled" },
         { "enabled",   "Enabled" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "vitaquakeii_xhair",
      "Show Crosshair",
      "Enables in game crosshair.",
      {
         { "disabled",  "Disabled" },
         { "enabled",   "Enabled" },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "vitaquakeii_invert_y_axis",
      "Invert Y Axis",
      "Invert the gamepad right analog stick's Y axis.",
      {
         { "disabled",  "Disabled" },
         { "enabled",   "Enabled" },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "vitaquakeii_fps",
      "Show FPS",
      "Shows framerate on top right screen.",
      {
         { "disabled",  "Disabled" },
         { "enabled",   "Enabled" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "vitaquakeii_hand",
      "Weapon Position",
      "Change positioning for the held weapon.",
      {
         { "right",  "Right" },
         { "left",   "Left" },
		 { "center", "Center" },
		 { "hidden", "Hidden" },
         { NULL, NULL },
      },
      "right"
   },
   { NULL, NULL, NULL, {{0}}, NULL },
};

/* RETRO_LANGUAGE_JAPANESE */

/* RETRO_LANGUAGE_FRENCH */

/* RETRO_LANGUAGE_SPANISH */

/* RETRO_LANGUAGE_GERMAN */

/* RETRO_LANGUAGE_ITALIAN */
struct retro_core_option_definition option_defs_it[] = {
   {
      "vitaquakeii_framerate",
      "Framerate (riavvio)",
      "Modifica il framerate. Richiede un riavvio.",
      {
         { "auto",            "Auto"},
         { "50",              "50fps"},
         { "60",              "60fps"},
         { "72",              "72fps"},
         { "75",              "75fps"},
         { "90",              "90fps"},
         { "100",              "100fps"},
         { "119",              "119fps"},
         { "120",              "120fps"},
         { "144",              "144fps"},
         { "155",              "155fps"},
         { "160",              "160fps"},
         { "165",              "165fps"},
         { "180",              "180fps"},
         { "200",              "200fps"},
         { "240",              "240fps"},
         { "244",              "244fps"},
         { NULL, NULL },
      },
      "auto"
   },
   {
      "vitaquakeii_resolution",
      "Risoluzione interna (riavvio)",
      "Configura la risoluzione. Richiede un riavvio.",
      {
         { "320x240",   NULL },
         { "480x272",   NULL },
         { "640x368",   NULL },
         { "640x480",   NULL },
         { "720x408",   NULL },
         { "960x544",   NULL },
         { "960x720",   NULL },
         { "1280x720",   NULL },
         { "1280x960",   NULL },
         { "1600x1200",   NULL },
         { "1920x1080",   NULL },
         { "1920x1440",   NULL },
         { "2240x1680",   NULL },
         { "2560x1440",   NULL },
         { "2560x1920",   NULL },
         { "2880x2160",   NULL },
         { "3200x2400",   NULL },
         { "3520x2640",   NULL },
         { "3840x2160",   NULL },
         { "3840x2880",   NULL },
         { "4160x3120",   NULL },
         { "4480x3360",   NULL },
         { "4800x3600",   NULL },
         { "5120x2880",   NULL },
         { "5120x3840",   NULL },
         { "5440x4080",   NULL },
         { "5760x4320",   NULL },
         { "6080x4560",   NULL },
         { "6400x4800",   NULL },
         { "6720x5040",   NULL },
         { "7040x5280",   NULL },
         { "7360x5520",   NULL },
         { "7680x4320",   NULL },
         { "7680x5760",   NULL },
         { "8000x6000",   NULL },
         { "8320x6240",   NULL },
         { "8640x6480",   NULL },
         { "8960x6720",   NULL },
         { "9280x6960",   NULL },
         { "9600x7200",   NULL },
         { "9920x7440",   NULL },
         { "10240x7680",   NULL },
         { "10560x7920",   NULL },
         { "10880x8160",   NULL },
         { "11200x8400",   NULL },
         { "11520x8640",   NULL },
         { "11840x8880",   NULL },
         { "12160x9120",   NULL },
         { "12480x9360",   NULL },
         { "12800x9600",   NULL },
         { "13120x9840",   NULL },
         { "13440x10080",   NULL },
         { "13760x10320",   NULL },
         { "14080x10560",   NULL },
         { "14400x10800",   NULL },
         { "14720x11040",   NULL },
         { "15040x11280",   NULL },
         { "15360x8640",   NULL },
         { "15360x11520",   NULL },
         { NULL, NULL },
      },
      "960x544"
   },
   {
      "vitaquakeii_dithered_filtering",
      "Filtro Dithering",
      "Abilita filtraggio software kernel-based per le texture. Riduce le performance. Funziona solo con renderer software.",
      {
         { "disabled",  "Disattivato" },
         { "enabled",   "Attivato" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "vitaquakeii_shadows",
      "Ombre Dinamiche",
      "Abilita il rendering delle ombre dinamiche. Funziona solo con renderer hardware.",
      {
         { "disabled",  "Disattivato" },
         { "enabled",   "Attivato" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "vitaquakeii_rumble",
      "Vibrazione",
      "Abilita la vibrazione del joypad.",
      {
         { "disabled",  "Disattivata" },
         { "enabled",   "Attivata" },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "vitaquakeii_specular",
      "Modalità Speculare",
      "Rende tutti i livelli di gioco speculari. Funziona solo con renderer hardware.",
      {
         { "disabled",  "Disattivata" },
         { "enabled",   "Attivata" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "vitaquakeii_xhair",
      "Mostra Mirino",
      "Abilita il mirino in gioco.",
      {
         { "disabled",  "Disattivato" },
         { "enabled",   "Attivato" },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "vitaquakeii_invert_y_axis",
      "Inverti Asse Y",
      "Inverte l'asse Y dell'analogico destro.",
      {
         { "disabled",  "Disattivato" },
         { "enabled",   "Attivato" },
         { NULL, NULL },
      },
      "enabled"
   },
   {
      "vitaquakeii_fps",
      "Mostra FPS",
      "Mostra il framerate nell'angolo in alto a destra dello schermo.",
      {
         { "disabled",  "Disattivato" },
         { "enabled",   "Attivato" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "vitaquakeii_hand",
      "Posizione dell'Arma",
      "Cambia la posizione dell'arma in uso.",
      {
         { "right",  "Destra" },
         { "left",   "Sinistra" },
		 { "center", "Centro" },
		 { "hidden", "Nascosta" },
         { NULL, NULL },
      },
      "right"
   },
   { NULL, NULL, NULL, {{0}}, NULL },
};

/* RETRO_LANGUAGE_DUTCH */

/* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */

/* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */

/* RETRO_LANGUAGE_RUSSIAN */

/* RETRO_LANGUAGE_KOREAN */

/* RETRO_LANGUAGE_CHINESE_TRADITIONAL */

/* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */

/* RETRO_LANGUAGE_ESPERANTO */

/* RETRO_LANGUAGE_POLISH */

/* RETRO_LANGUAGE_VIETNAMESE */

/* RETRO_LANGUAGE_ARABIC */

/* RETRO_LANGUAGE_GREEK */

/* RETRO_LANGUAGE_TURKISH */

/*
 ********************************
 * Language Mapping
 ********************************
*/

struct retro_core_option_definition *option_defs_intl[RETRO_LANGUAGE_LAST] = {
   option_defs_us, /* RETRO_LANGUAGE_ENGLISH */
   NULL,           /* RETRO_LANGUAGE_JAPANESE */
   NULL,           /* RETRO_LANGUAGE_FRENCH */
   NULL,           /* RETRO_LANGUAGE_SPANISH */
   NULL,           /* RETRO_LANGUAGE_GERMAN */
   option_defs_it, /* RETRO_LANGUAGE_ITALIAN */
   NULL,           /* RETRO_LANGUAGE_DUTCH */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   NULL,           /* RETRO_LANGUAGE_RUSSIAN */
   NULL,           /* RETRO_LANGUAGE_KOREAN */
   NULL,           /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   NULL,           /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   NULL,           /* RETRO_LANGUAGE_ESPERANTO */
   NULL,           /* RETRO_LANGUAGE_POLISH */
   NULL,           /* RETRO_LANGUAGE_VIETNAMESE */
   NULL,           /* RETRO_LANGUAGE_ARABIC */
   NULL,           /* RETRO_LANGUAGE_GREEK */
   NULL,           /* RETRO_LANGUAGE_TURKISH */
};

/*
 ********************************
 * Functions
 ********************************
*/

/* Handles configuration/setting of core options.
 * Should only be called inside retro_set_environment().
 * > We place the function body in the header to avoid the
 *   necessity of adding more .c files (i.e. want this to
 *   be as painless as possible for core devs)
 */

static INLINE void libretro_set_core_options(retro_environment_t environ_cb)
{
   unsigned version = 0;

   if (!environ_cb)
      return;

   if (environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && (version == 1))
   {
      struct retro_core_options_intl core_options_intl;
      unsigned language = 0;

      core_options_intl.us    = option_defs_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
          (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = option_defs_intl[language];

      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_intl);
   }
   else
   {
      size_t i;
      size_t num_options               = 0;
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine number of options */
      while (true)
      {
         if (option_defs_us[num_options].key)
            num_options++;
         else
            break;
      }

      /* Allocate arrays */
      variables  = (struct retro_variable *)calloc(num_options + 1, sizeof(struct retro_variable));
      values_buf = (char **)calloc(num_options, sizeof(char *));

      if (!variables || !values_buf)
         goto error;

      /* Copy parameters from option_defs_us array */
      for (i = 0; i < num_options; i++)
      {
         const char *key                        = option_defs_us[i].key;
         const char *desc                       = option_defs_us[i].desc;
         const char *default_value              = option_defs_us[i].default_value;
         struct retro_core_option_value *values = option_defs_us[i].values;
         size_t buf_len                         = 3;
         size_t default_index                   = 0;

         values_buf[i] = NULL;

         if (desc)
         {
            size_t num_values = 0;

            /* Determine number of values */
            while (true)
            {
               if (values[num_values].value)
               {
                  /* Check if this is the default value */
                  if (default_value)
                     if (strcmp(values[num_values].value, default_value) == 0)
                        default_index = num_values;

                  buf_len += strlen(values[num_values].value);
                  num_values++;
               }
               else
                  break;
            }

            /* Build values string */
            if (num_values > 1)
            {
               size_t j;

               buf_len += num_values - 1;
               buf_len += strlen(desc);

               values_buf[i] = (char *)calloc(buf_len, sizeof(char));
               if (!values_buf[i])
                  goto error;

               strcpy(values_buf[i], desc);
               strcat(values_buf[i], "; ");

               /* Default value goes first */
               strcat(values_buf[i], values[default_index].value);

               /* Add remaining values */
               for (j = 0; j < num_values; j++)
               {
                  if (j != default_index)
                  {
                     strcat(values_buf[i], "|");
                     strcat(values_buf[i], values[j].value);
                  }
               }
            }
         }

         variables[i].key   = key;
         variables[i].value = values_buf[i];
      }
      
      /* Set variables */
      environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);

error:

      /* Clean up */
      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif
