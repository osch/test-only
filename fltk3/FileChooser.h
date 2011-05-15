//
// "$Id: FileChooser.h 8286 2011-01-17 09:39:14Z manolo $"
//
// Fl_File_Chooser dialog for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

// generated by Fast Light User Interface Designer (fluid) version 1.0300

#ifndef Fl_File_Chooser_H
#define Fl_File_Chooser_H
#include <fltk3/run.h>
#include <fltk3/DoubleWindow.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fltk3/Group.h>
#include <fltk3/Choice.h>
#include <fltk3/MenuButton.h>
#include <fltk3/Button.h>
#include <fltk3/Preferences.h>
#include <fltk3/TiledGroup.h>
#include <fltk3/FileBrowser.h>
#include <fltk3/Box.h>
#include <fltk3/CheckButton.h>
#include <fltk3/FileInput.h>
#include <fltk3/ReturnButton.h>
#include <fltk3/ask.h>

class FL_EXPORT Fl_File_Chooser {
public:
  enum { SINGLE = 0, MULTI = 1, CREATE = 2, DIRECTORY = 4 }; 
private:
  static Fl_Preferences prefs_; 
  void (*callback_)(Fl_File_Chooser*, void *); 
  void *data_; 
  char directory_[FL_PATH_MAX]; 
  char pattern_[FL_PATH_MAX]; 
  char preview_text_[2048]; 
  int type_; 
  void favoritesButtonCB(); 
  void favoritesCB(Fl_Widget *w); 
  void fileListCB(); 
  void fileNameCB(); 
  void newdir(); 
  static void previewCB(Fl_File_Chooser *fc); 
  void showChoiceCB(); 
  void update_favorites(); 
  void update_preview(); 
public:
  Fl_File_Chooser(const char *d, const char *p, int t, const char *title);
private:
  Fl_Double_Window *window;
  void cb_window_i(Fl_Double_Window*, void*);
  static void cb_window(Fl_Double_Window*, void*);
  Fl_Choice *showChoice;
  void cb_showChoice_i(Fl_Choice*, void*);
  static void cb_showChoice(Fl_Choice*, void*);
  Fl_Menu_Button *favoritesButton;
  void cb_favoritesButton_i(Fl_Menu_Button*, void*);
  static void cb_favoritesButton(Fl_Menu_Button*, void*);
public:
  Fl_Button *newButton;
private:
  void cb_newButton_i(Fl_Button*, void*);
  static void cb_newButton(Fl_Button*, void*);
  void cb__i(Fl_Tile*, void*);
  static void cb_(Fl_Tile*, void*);
  Fl_File_Browser *fileList;
  void cb_fileList_i(Fl_File_Browser*, void*);
  static void cb_fileList(Fl_File_Browser*, void*);
  Fl_Box *previewBox;
public:
  Fl_Check_Button *previewButton;
private:
  void cb_previewButton_i(Fl_Check_Button*, void*);
  static void cb_previewButton(Fl_Check_Button*, void*);
  Fl_File_Input *fileName;
  void cb_fileName_i(Fl_File_Input*, void*);
  static void cb_fileName(Fl_File_Input*, void*);
  Fl_Return_Button *okButton;
  void cb_okButton_i(Fl_Return_Button*, void*);
  static void cb_okButton(Fl_Return_Button*, void*);
  Fl_Button *cancelButton;
  void cb_cancelButton_i(Fl_Button*, void*);
  static void cb_cancelButton(Fl_Button*, void*);
  Fl_Double_Window *favWindow;
  Fl_File_Browser *favList;
  void cb_favList_i(Fl_File_Browser*, void*);
  static void cb_favList(Fl_File_Browser*, void*);
  Fl_Button *favUpButton;
  void cb_favUpButton_i(Fl_Button*, void*);
  static void cb_favUpButton(Fl_Button*, void*);
  Fl_Button *favDeleteButton;
  void cb_favDeleteButton_i(Fl_Button*, void*);
  static void cb_favDeleteButton(Fl_Button*, void*);
  Fl_Button *favDownButton;
  void cb_favDownButton_i(Fl_Button*, void*);
  static void cb_favDownButton(Fl_Button*, void*);
  Fl_Button *favCancelButton;
  void cb_favCancelButton_i(Fl_Button*, void*);
  static void cb_favCancelButton(Fl_Button*, void*);
  Fl_Return_Button *favOkButton;
  void cb_favOkButton_i(Fl_Return_Button*, void*);
  static void cb_favOkButton(Fl_Return_Button*, void*);
#ifndef WIN32
  Fl_Check_Button *show_hidden;
  static void show_hidden_cb(Fl_Check_Button*, void*);
  void remove_hidden_files();
#endif
public:
  ~Fl_File_Chooser();
  void callback(void (*cb)(Fl_File_Chooser *, void *), void *d = 0);
  void color(Fl_Color c);
  Fl_Color color();
  int count(); 
  void directory(const char *d); 
  char * directory();
  void filter(const char *p); 
  const char * filter();
  int filter_value();
  void filter_value(int f);
  void hide();
  void iconsize(uchar s);
  uchar iconsize();
  void label(const char *l);
  const char * label();
  void ok_label(const char *l);
  const char * ok_label();
  void preview(int e); 
  int preview() const { return previewButton->value(); }; 
  void rescan(); 
  void rescan_keep_filename(); 
  void show();
  int shown();
  void textcolor(Fl_Color c);
  Fl_Color textcolor();
  void textfont(Fl_Font f);
  Fl_Font textfont();
  void textsize(Fl_Fontsize s);
  Fl_Fontsize textsize();
  void type(int t);
  int type();
  void * user_data() const;
  void user_data(void *d);
  const char *value(int f = 1); 
  void value(const char *filename); 
  int visible();
  /**
     [standard text may be customized at run-time]
  */
  static const char *add_favorites_label; 
  /**
     [standard text may be customized at run-time]
  */
  static const char *all_files_label; 
  /**
     [standard text may be customized at run-time]
  */
  static const char *custom_filter_label; 
  /**
     [standard text may be customized at run-time]
  */
  static const char *existing_file_label; 
  /**
     [standard text may be customized at run-time]
  */
  static const char *favorites_label; 
  /**
     [standard text may be customized at run-time]
  */
  static const char *filename_label; 
  /**
     [standard text may be customized at run-time]
  */
  static const char *filesystems_label; 
  /**
     [standard text may be customized at run-time]
  */
  static const char *manage_favorites_label; 
  /**
     [standard text may be customized at run-time]
  */
  static const char *new_directory_label; 
  /**
     [standard text may be customized at run-time]
  */
  static const char *new_directory_tooltip; 
  /**
     [standard text may be customized at run-time]
  */
  static const char *preview_label; 
  /**
     [standard text may be customized at run-time]
  */
  static const char *save_label; 
  /**
     [standard text may be customized at run-time]
  */
  static const char *show_label; 
  /** 
     [standard text may be customized at run-time]
 */
  static const char *hidden_label;
  /**
     the sort function that is used when loading
     the contents of a directory.
  */
  static Fl_File_Sort_F *sort; 
private:
  Fl_Widget* ext_group; 
public:
  Fl_Widget* add_extra(Fl_Widget* gr);
  Fl_File_Browser *browser(void) {return fileList; };
};
FL_EXPORT char *fl_dir_chooser(const char *message,const char *fname,int relative=0);
FL_EXPORT char *fl_file_chooser(const char *message,const char *pat,const char *fname,int relative=0);
FL_EXPORT void fl_file_chooser_callback(void (*cb)(const char*));
FL_EXPORT void fl_file_chooser_ok_label(const char*l);
#endif

//
// End of "$Id: FileChooser.h 8286 2011-01-17 09:39:14Z manolo $".
//
