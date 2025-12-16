#include <gtk/gtk.h>
#include <stdlib.h>


// paths to images
#define INPUT_IMAGE   "data/input/level_2_image_2.png"
#define ROTATED_IMAGE "data/images/treated_image.png"
#define SOLVED_IMAGE  "output.png"


GtkWidget *image_widget;


void on_rotate_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;

    // call automatic rotation
    int e = system("./pretreatment level_2_image_2.png");
    if(e != 0){ printf("rotate syst error"); return;}

    gtk_image_set_from_file(GTK_IMAGE(image_widget), ROTATED_IMAGE);
}

void on_solve_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;

    // run OCR and solver
    int e = system("./wordsearch_solver data/images/treated_image.png");
    if(e != 0){ printf("solver syst error"); return;}

    gtk_image_set_from_file(GTK_IMAGE(image_widget), SOLVED_IMAGE);
}


int main(int argc, char *argv[])
{

    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *rotate_button;
    GtkWidget *solve_button;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Word Search Solver");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    image_widget = gtk_image_new_from_file(INPUT_IMAGE);
    gtk_box_pack_start(GTK_BOX(vbox), image_widget, TRUE, TRUE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    // rotate button
    rotate_button = gtk_button_new_with_label("Rotate Image");
    g_signal_connect(rotate_button, "clicked",
                     G_CALLBACK(on_rotate_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), rotate_button, TRUE, TRUE, 0);

    // solver button
    solve_button = gtk_button_new_with_label("Solve");
    g_signal_connect(solve_button, "clicked",
                     G_CALLBACK(on_solve_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), solve_button, TRUE, TRUE, 0);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
 