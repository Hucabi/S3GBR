#include <gtk/gtk.h>
#include <stdlib.h>


/* Paths to images */
#define INPUT_IMAGE   "data/images/input.png"
#define ROTATED_IMAGE "data/images/02_corrected_final.png"
#define SOLVED_IMAGE  "../ouput/output.png"

/* Global image widget */
GtkWidget *image_widget;

/* ---------- Button callbacks ---------- */

void on_rotate_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;

    // call automatic rotation
    int e = system("./pretreatment level_2_image_2.png");
    if(e != 0){ printf("rotate syst error"); return;}

    /* Update displayed image */
    gtk_image_set_from_file(GTK_IMAGE(image_widget), ROTATED_IMAGE);
}

void on_solve_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;

    // run OCR and solver
    int e = system("./wordsearch_solver data/images/treated_image.png");
    if(e != 0){ printf("solver syst error"); return;}

    /* Display solved grid */
    gtk_image_set_from_file(GTK_IMAGE(image_widget), SOLVED_IMAGE);
}

/* ---------- Main ---------- */

int main(int argc, char *argv[])
{

    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *rotate_button;
    GtkWidget *solve_button;

    gtk_init(&argc, &argv);

    /* Window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Word Search Solver");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* Vertical layout */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    /* Image display */
    image_widget = gtk_image_new_from_file(INPUT_IMAGE);
    gtk_box_pack_start(GTK_BOX(vbox), image_widget, TRUE, TRUE, 0);

    /* Horizontal box for buttons */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* Rotate button */
    rotate_button = gtk_button_new_with_label("Rotate Image");
    g_signal_connect(rotate_button, "clicked",
                     G_CALLBACK(on_rotate_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), rotate_button, TRUE, TRUE, 0);

    /* Solve button */
    solve_button = gtk_button_new_with_label("Solve");
    g_signal_connect(solve_button, "clicked",
                     G_CALLBACK(on_solve_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), solve_button, TRUE, TRUE, 0);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
 