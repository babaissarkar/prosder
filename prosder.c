/*
 * prosder.c
 * 
 * Copyright 2020 Subhraman Sarkar <subhraman@subhraman-Inspiron>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */
 
 /* Command based graphics like LOGO */
 /* Dated : 19.08.2018 */

#include <stdio.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>
#include <cairo/cairo-svg.h>

GtkWidget *img;
GdkPixbuf *pix, *pix2;
GtkWidget *text;

cairo_surface_t *surf;
cairo_t *ct;

int w, h; /* Width and Height of the Canvas */
int curX, curY; /* Current position */
double lw; /* Line Width/Stroke Width */
double r, g, b; /* RGB color values */
int lcount;
double Tr[2][2];
int dtx, dty;

int has_loop, first_iter;
int loop_start, loop_end, loop_depth, repeat;
/* loop_start -> Start of loop
 * loop_end -> End of loop
 * has_loop -> Inside a loop or not
 * first_iter -> Is this the first iteration of the loop?
 * repeat -> How many times the loop will repeat.
 * loop_depth -> Number of outer loops inside which this loop is nested.*/
int show_grid;

int curr_surf_type;
	
void choose_color();
void draw_arc(double cx, double cy, double rad, double ang1, double ang2);
void draw_init(int type);
void draw_line(int x1, int y1);
void get_commands();
int get_XY(int x, int y, int choice);
void move_cursor(int x1, int y1);
void parse_command(gchar *cmd, int count);
void update_canvas();
void write_to_svg();


void move_cursor(int x1, int y1)
{
	/* No need to bother with cairo! */
	curX = x1;
	curY = y1;
}

void move_cursor_relative(int dx, int dy)
{
	curX += dx;
	curY += dy;
}

void translate(int dx, int dy)
{
	/*cairo_translate(ct, dx, dy);*/
}

void rotate(int deg)
{
	double rad = (deg * M_PI)/180.0;
	double Tr1[2][2];
	/* Subsequent transformations add */
	Tr1[0][0] = cos(rad) * Tr[0][0] - sin(rad) * Tr[1][0];
	Tr1[0][1] = cos(rad) * Tr[0][1] - sin(rad) * Tr[1][1];
	Tr1[1][0] = sin(rad) * Tr[0][0] + cos(rad) * Tr[1][0];
	Tr1[1][1] = sin(rad) * Tr[0][1] + cos(rad) * Tr[1][1];

	Tr[0][0] = Tr1[0][0];
	Tr[0][1] = Tr1[0][1];
	Tr[1][0] = Tr1[1][0];
	Tr[1][1] = Tr1[1][1];
}

void draw_line(int x1, int y1)
{
	int X, Y;
	int X2, Y2;
	
	/* Set color and stoke width */
	cairo_set_source_rgb(ct, r, g, b);
	cairo_set_line_width(ct, lw);

	/* Translate from user-space coordinates to device space */
	/* Should move this to a function */
	X = get_XY(curX, curY, 0);
	Y = get_XY(curX, curY, 1);
	X2 = get_XY(x1, y1, 0);
	Y2 = get_XY(x1, y1, 1);

	/*printf("%d, %d, %d, %d\n", X, Y, X2, Y2);*/
	/* Move the current position of the cursor */
	curX = x1;
	curY = y1;

	/* Finally, draw the line */
	cairo_move_to(ct, X, Y);
	cairo_line_to(ct, X2, Y2);
	cairo_stroke(ct);

	/* Update */
	update_canvas();
}

void draw_arc(double cx, double cy, double rad, double ang1, double ang2)
{
	double th1, th2;
	double cx1, cy1;
	double X, Y;

	/* Set color and stoke width */
	cairo_set_source_rgb(ct, r, g, b);
	cairo_set_line_width(ct, lw);

	/* Translate coordinates */
	cx1 = get_XY(cx, cy, 0);
	cy1 = get_XY(cx, cy, 1);
	X = get_XY(curX, curY, 0);
	Y = get_XY(curX, curY, 1);
	th1 = (ang1 * M_PI)/180.0;
	th2 = (ang2 * M_PI)/180.0;

	/* Draw */
	/* Cairo draws counter clockwise, but we need clockwise arcs */
	cairo_arc(ct, cx1, cy1, rad, -th2, -th1);
	cairo_stroke(ct);

	/* Update current cursor position */
	curX = cx + rad*cos(th1);
	curY = cy - rad*sin(th1);

	/* Update */
	update_canvas();
}

int get_XY(int x, int y, int choice)
{
	/* Convert from user to device space coordinates */
	/* and also apply any transformations on the way. */
	int TX, TY;
	int R;
	TX = Tr[0][0]*x + Tr[0][1]*y; /* Apply transformation matrix Tr */
	TY = Tr[1][0]*x + Tr[1][1]*y;
	TX = TX + w/2 + dtx; /* Convert from user to device coordinates */
	TY = h/2 - (TY + dty); /* Taking into account any translation due to dtx and dty */
	
	/* Since I can't return both X and Y, I'm returning one of them
	 * depending on 'choice' variable. */
	R = (choice == 0) ? TX : TY;
	return R;
}

void parse_command(gchar *cmd, int count)
{
	gchar **toks;
	int x, y;
	int len;

	toks = g_strsplit(cmd, " ", -1);

	/* Variable Substitution */
	len = 0;

	/* Command Recognition */
	if (strcmp(toks[0], "LN") == 0)
	{
		g_print("CMD : Line");
		x = atoi(toks[1]);
		y = atoi(toks[2]);
		g_print("(%d, %d) to (%d, %d)\n", curX, curY, x, y);
		draw_line(x, y);
	}
	else if (strcmp(toks[0], "MV") == 0)
	{
		g_print("CMD : Move");
		x = atoi(toks[1]);
		y = atoi(toks[2]);
		g_print("(%d, %d) to (%d, %d)\n", curX, curY, x, y);
		move_cursor(x, y);
	}
	else if (strcmp(toks[0], "MVR") == 0)
	{
		g_print("CMD : Move Relative");
		x = atoi(toks[1]);
		y = atoi(toks[2]);
		g_print("(%d, %d) to (%d, %d)\n", curX, curY, curX+x, curY+y);
		move_cursor_relative(x, y);
	}
	else if (strcmp(toks[0], "CLR") == 0)
	{
		g_print("CMD : Clear Screen\n");
		draw_init(curr_surf_type);
	}
	else if (strcmp(toks[0], "COL") == 0)
	{
		g_print("CMD : FG Color");
		g_print("(%d, %d, %d)\n", atoi(toks[1]), atoi(toks[2]), atoi(toks[3]));
		r = atoi(toks[1])/255.0;
		g = atoi(toks[2])/255.0;
		b = atoi(toks[3])/255.0;
	}
	else if (strcmp(toks[0], "LW") == 0)
	{
		g_print("CMD : Stroke Width");
		g_print("(%3.1f)\n", atof(toks[1]));
		lw = atof(toks[1]);
	}
	else if (strcmp(toks[0], "ARC") == 0)
	{
		double cx, cy;
		double ang1, ang2;
		double rad;
		cx = atof(toks[1]);
		cy = atof(toks[2]);
		rad = atof(toks[3]);
		ang1 = atof(toks[4]);
		ang2 = atof(toks[5]);
		
		g_print("CMD : Arc ");
		g_print("[CEN=(%4.2f, %4.2f), ", cx, cy);
		g_print("RAD=%4.1f, ", rad);
		g_print("ANG1=%6.3f deg, ANG2=%6.3f deg]\n", ang1, ang2);

		draw_arc(cx, cy, rad, ang1, ang2);
		
	} /*Now looping commands, the most difficult part*/
	else if (strcmp(toks[0], "LOOP") == 0)
	{
		/* Recording the line no. from where loop will start*/
		if (!has_loop)
		{
			has_loop = 1;
			loop_start = count;
			repeat = atoi(toks[1]);
			if (first_iter == 0) {
				first_iter = 1;
			}
			++loop_depth;
		}
	}
	else if (strcmp(toks[0], "LEND") == 0)
	{
		if (has_loop)
		{
			loop_end = count;
			if (repeat > 0)
			{
				--repeat;
			}
			else if (repeat == 0)
			{
				first_iter = 0;
				has_loop = 0;
				--loop_depth;
			}
		}
	}
	else if (strcmp(toks[0], "GRID") == 0)
	{
		if(strcmp(toks[1], "ON") == 0)
		{
			show_grid = 1;
			g_print("CMD : Grid ON\n");
		}
		else if(strcmp(toks[1], "OFF") == 0)
		{
			show_grid = 0;
			g_print("CMD : Grid OFF\n");
		}
		draw_init(curr_surf_type);
	}
	else if (strcmp(toks[0], "TRN") == 0)
	{
		dtx += atoi(toks[1]);
		dty += atoi(toks[2]);
		g_print("CMD : Translate");
		g_print("(%d, %d)\n", dtx, dty);
	}
	else if (strcmp(toks[0], "ROT") == 0)
	{
		int deg;
		deg = atoi(toks[1]);
		rotate(deg);
		g_print("CMD : Rotate %d Degrees CCW\n", deg);
		g_print("[%4.2f, %4.2f\n", Tr[0][0], Tr[0][1]);
		g_print("%4.2f, %4.2f]\n", Tr[1][0], Tr[1][1]);
	}
}

void get_commands()
{
	GtkTextIter start, end;
	GtkTextBuffer *bf;
	gchar *cmdtext, **cmds;
	gint no_of_lines;
	int i;

	/* Get the commmands as text */	
	bf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	gtk_text_buffer_get_bounds(bf, &start, &end);
	cmdtext = gtk_text_buffer_get_text(bf, &start, &end, FALSE);

	/* Then split them into lines */
	cmds = g_strsplit(cmdtext, "\n", -1);
	no_of_lines = gtk_text_buffer_get_line_count(bf);
	/*g_print("%d, %s\n", no_of_lines, cmds[0]);*/

	/* And parse */
	for(i = 0; i < no_of_lines; i++)
	{
		/*printf("<%d> ", i);*/
		parse_command(cmds[i], i);
		if ((has_loop) && (repeat > 0) && (i == loop_end))
		{
			i = loop_start;
		}
	}
}

static void init_svg(GtkWidget *widget, gpointer data)
{
	draw_init(1); /* Initialize SVG surface */
}

void init_cairo_surface()
{
	surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 500, 400);
	ct = cairo_create(surf);
}

void init_svg_surface()
{
	surf = cairo_svg_surface_create(".drawing.svg", 500, 400);
	ct = cairo_create(surf);
}

static void finalize_svg_surface(GtkWidget *widget, gpointer win)
{
	cairo_surface_flush(surf);

	/* Move drawing to actual location */
	GtkWidget *files;
    gchar *flname;
    int stat;
    /* Open the file */
    g_print("Saving File...\n");
    files = gtk_file_chooser_dialog_new
    (
    "Save file", win, GTK_FILE_CHOOSER_ACTION_SAVE,
    "Cancel", GTK_RESPONSE_CANCEL,
    "Save", GTK_RESPONSE_ACCEPT, NULL
    );
    stat = gtk_dialog_run(GTK_DIALOG(files));
    if (stat == GTK_RESPONSE_ACCEPT)
    {
		flname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(files));
		rename(".drawing.svg", flname);
		g_print("%s\n", flname);
	}
	gtk_widget_destroy(GTK_WIDGET(files));
}

/*void write_to_svg()
{
	int width, height;
	// Get the Cairo surface from the global Pixbuf pix
	cairo_surface_t* svg_surf;
	cairo_t* svg_ct;
	
	pix = gtk_image_get_pixbuf(GTK_IMAGE(img));
	width = gdk_pixbuf_get_width(pix);
	height = gdk_pixbuf_get_height(pix);
	svg_surf = cairo_svg_surface_create("drawing.svg", width, height);
	cairo_svg_surface_restrict_to_version(svg_surf, CAIRO_SVG_VERSION_1_2);
	svg_ct = cairo_create(svg_surf);
	// Paint the pixbuf onto the surface
	cairo_set_source_surface(svg_ct, surf, 0, 0);
	cairo_rectangle(svg_ct, 0, 0, width, height);
	cairo_fill(svg_ct);
	cairo_destroy(svg_ct);
	cairo_surface_flush(svg_surf);
	cairo_surface_finish(svg_surf);
	cairo_surface_destroy(svg_surf);
}*/

void update_canvas()
{
	pix = gdk_pixbuf_get_from_surface(surf, 0, 0, w, h);
	gtk_image_set_from_pixbuf(GTK_IMAGE(img), pix);
}

void destroy_canvas()
{
	cairo_destroy(ct);
	cairo_surface_destroy(surf);
}

void draw_borders()
{
	cairo_set_source_rgb(ct, 1.0, 1.0, 1.0);
	cairo_rectangle(ct, 0, 0, w, h);
	cairo_fill(ct);
	
	cairo_set_source_rgb(ct, 0.0, 0.0, 0.0);
	cairo_set_line_width(ct, 8.0);
	cairo_rectangle(ct, 0, 0, w, h);
	cairo_stroke(ct);
}

void draw_grid()
{
	if (show_grid == 1)
	{
		double dashes[1] = {2};
		cairo_set_line_width(ct, 1.0);
		cairo_set_dash(ct, dashes, 1, 0);
		cairo_set_line_width(ct, 2.0);
		cairo_move_to(ct, 0, h/2);
		cairo_line_to(ct, w, h/2);
		cairo_move_to(ct, w/2, 0);
		cairo_line_to(ct, w/2, h);
		cairo_stroke(ct);
	}
}


void draw_init(int type)
{
	/* Initiatilize parameters and color */
	r = 0.0; g = 0.0; b = 0.0;
	lw = 2.0;
	curX = 0;
	curY = 0;
	Tr[0][0] = 1;
	Tr[0][1] = 0;
	Tr[1][0] = 0;
	Tr[1][1] = 1;
	dtx = 0; dty = 0;

	/* Loop Variables */
	has_loop = 0; /* FALSE */
	loop_start = -1;
	loop_end = -1;
	loop_depth = 0;
	repeat = 0;
	first_iter = 0; /* FALSE */

	curr_surf_type = type;

	switch(type)
	{
		case 0 :
			init_cairo_surface();
			break;
		case 1 :
			init_svg_surface();
			break;
	}
	draw_borders();
	draw_grid();

	/* Update */
	update_canvas();
}

void choose_color(GtkWidget *widget, gpointer win)
{
	char col_cmd[40];
	GtkWidget *gc;
	gint stat;
	GdkRGBA mycol;
	int r1, g1, b1;
	
	gc = gtk_color_chooser_dialog_new("Choose Colors", win);
	stat = gtk_dialog_run(GTK_DIALOG(gc));
	
	if (stat == GTK_RESPONSE_OK)
	{
		gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(gc), &mycol);
		r = mycol.red; g = mycol.green; b = mycol.blue;
		r1 = 255*r; g1 = 255*g; b1 = 255*b;
		g_print("CMD : FG Color");
		g_print("(%d, %d, %d)\n", r1, g1, b1);
	}
	
	gtk_widget_destroy(GTK_WIDGET(gc));
}

static void save(GtkWidget *widget, gpointer win)
{
    GtkWidget *files;
    gchar *flname;
    int stat;
    /* Open the file */
    g_print("Saving File...\n");
    files = gtk_file_chooser_dialog_new
    (
    "Save file", win, GTK_FILE_CHOOSER_ACTION_SAVE,
    "Cancel", GTK_RESPONSE_CANCEL,
    "Save", GTK_RESPONSE_ACCEPT, NULL
    );
    stat = gtk_dialog_run(GTK_DIALOG(files));
    if (stat == GTK_RESPONSE_ACCEPT)
    {
		flname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(files));
		pix = gtk_image_get_pixbuf(GTK_IMAGE(img));
		gdk_pixbuf_save(pix, flname, "png", NULL, NULL);
		g_print("%s\n", flname);
	}
	gtk_widget_destroy(GTK_WIDGET(files));
}

static void open(GtkWidget *widget, gpointer win)
{
    GtkWidget *files;
    gchar *flname;
    int stat;
    /* Open the file */
    g_print("Opening File...\n");
    files = gtk_file_chooser_dialog_new
    (
    "Open file", win, GTK_FILE_CHOOSER_ACTION_OPEN,
    "Cancel", GTK_RESPONSE_CANCEL,
    "Open", GTK_RESPONSE_ACCEPT, NULL
    );
    /*gtk_window_set_default_size(GTK_WINDOW(files), 700, 400);*/
    stat = gtk_dialog_run(GTK_DIALOG(files));
    if (stat == GTK_RESPONSE_ACCEPT) {
		
        flname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(files));
        pix = gdk_pixbuf_new_from_file_at_scale(flname, w, h, TRUE, NULL);
        gtk_image_set_from_pixbuf(GTK_IMAGE(img), pix);
        g_print("%s\n", flname);
    }
    gtk_widget_destroy(GTK_WIDGET(files));
}

static void run(GtkWidget *widget, gpointer win)
{
	get_commands();
}

static void clear(GtkWidget *widget, gpointer win)
{
	g_print("CMD : Clear Screen\n");
	draw_init(curr_surf_type);
}

static void exitApp(GtkWidget *widget, gpointer dat)
{
	destroy_canvas();
	g_application_quit(dat);
}

static void activate(GtkApplication *gapp, gpointer data)
{
    GtkWidget *window, *box, *box2, *scroll, *frame;
    GtkWidget *menubar, *mnFile, *mnDraw, *mnExit, *mnOpen, *mnSave, *mnSVG, *mnNewSVG, *mnSaveSVG, *mnExec, *mnClear, *mnColor;
    GtkWidget *mnuFile, *mnuDraw, *mnuSVG;
    GtkWidget *files;
    GdkPixbuf *icon;
    GtkAccelGroup *aclg;
    
    /* Initializing main window. */
    icon = gdk_pixbuf_new_from_file("prosder.svg", NULL);
    window = gtk_application_window_new(gapp);
    gtk_window_set_title(GTK_WINDOW(window), "Prosder");
    gtk_window_set_default_size(GTK_WINDOW(window), w+100, h+160);
    gtk_window_set_icon(GTK_WINDOW(window), icon);

    aclg = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), aclg);

	/* The Canvas */
	img = gtk_image_new_from_icon_name("edit-delete", GTK_ICON_SIZE_DIALOG);
	gtk_widget_set_size_request(img, w+50, h);
    
    /* The command window. */
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(window), box);

    box2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 150);
    text = gtk_text_view_new();

	gtk_widget_set_margin_start(box2, 10);
    gtk_widget_set_margin_end(box2, 10);
    gtk_widget_set_margin_bottom(box2, 10);
    
    gtk_container_add(GTK_CONTAINER(scroll), text);

	frame = gtk_frame_new("Commands");
    gtk_container_add(GTK_CONTAINER(frame), scroll);
    gtk_container_add(GTK_CONTAINER(box2), frame);
    
    /* Setting up the Menu. */
    menubar = gtk_menu_bar_new();
    mnFile = gtk_menu_item_new_with_label("File");
    mnSVG = gtk_menu_item_new_with_label("SVG");
    mnDraw = gtk_menu_item_new_with_label("Draw");

    mnuFile = gtk_menu_new();
    mnuDraw = gtk_menu_new();
    mnuSVG = gtk_menu_new();

    mnOpen = gtk_menu_item_new_with_label("Open");
    mnExec = gtk_menu_item_new_with_label("Execute");
    mnSave = gtk_menu_item_new_with_label("Save");
    mnNewSVG = gtk_menu_item_new_with_label("New SVG Image...");
    mnSaveSVG = gtk_menu_item_new_with_label("Save to SVG...");
    mnExit = gtk_menu_item_new_with_label("Exit");
    mnColor = gtk_menu_item_new_with_label("Color...");
    mnClear = gtk_menu_item_new_with_label("Clear Screen");

    g_signal_connect(mnOpen, "activate", G_CALLBACK(open), window);
    g_signal_connect(mnSave, "activate", G_CALLBACK(save), window);
    g_signal_connect(mnNewSVG, "activate", G_CALLBACK(init_svg), window);
    g_signal_connect(mnSaveSVG, "activate", G_CALLBACK(finalize_svg_surface), window);
    g_signal_connect(mnExec, "activate", G_CALLBACK(run), window);
    g_signal_connect(mnExit, "activate", G_CALLBACK(exitApp), G_APPLICATION(gapp));
    g_signal_connect(mnColor, "activate", G_CALLBACK(choose_color), window);
	g_signal_connect(mnClear, "activate", G_CALLBACK(clear), window);

	gtk_widget_add_accelerator(mnExec, "activate", aclg, GDK_KEY_e, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    gtk_menu_attach(GTK_MENU(mnuFile), mnOpen, 0, 1, 0, 1);
    gtk_menu_attach(GTK_MENU(mnuFile), mnSave, 0, 1, 1, 2);
    gtk_menu_attach(GTK_MENU(mnuFile), mnExec, 0, 1, 2, 3);
    gtk_menu_attach(GTK_MENU(mnuFile), mnExit, 0, 1, 3, 4);
    gtk_menu_attach(GTK_MENU(mnuDraw), mnColor, 0, 1, 0, 1);
    gtk_menu_attach(GTK_MENU(mnuDraw), mnClear, 0, 1, 1, 2);
    gtk_menu_attach(GTK_MENU(mnuSVG), mnNewSVG, 0, 1, 0, 1);
    gtk_menu_attach(GTK_MENU(mnuSVG), mnSaveSVG, 0, 1, 1, 2);
    
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mnFile), mnuFile);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mnDraw), mnuDraw);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mnSVG), mnuSVG);
    
    gtk_container_add(GTK_CONTAINER(menubar), mnFile);
    gtk_container_add(GTK_CONTAINER(menubar), mnSVG);
    gtk_container_add(GTK_CONTAINER(menubar), mnDraw);
    
    /* Adding widget to main window and showing. */
    
    gtk_container_add(GTK_CONTAINER(box), menubar);
    gtk_container_add(GTK_CONTAINER(box), img);
    gtk_container_add(GTK_CONTAINER(box), box2);
    
    gtk_widget_show_all(window);

	draw_init(0); /* Default PNG surface */
}

int main(int argc, char **argv)
{
	GtkApplication *gapp;
    int stat;

	/* Intializing parameters */
	w = 500; h = 400;
	show_grid = 0; /* FALSE */
	
    /* Start GUI */
    gapp = gtk_application_new("com.babai.prosder", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(gapp, "activate", G_CALLBACK(activate), NULL);
    stat = g_application_run(G_APPLICATION(gapp), argc, argv);
    g_object_unref(gapp);
    
	return stat;
}
