/* 
 * expense/Pref.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 * Copyright (C) 2001 David.Goodenough@DGA.co.uk
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

package org.gnu.pdapilot.expense;

import java.io.*;

public class Pref extends org.gnu.pdapilot.Pref {
   private int currentCategory, defaultCategory;
   private int noteFont;
   private boolean showAllCategories;
   private boolean showCurrency;
   private boolean saveBackup;
   private boolean allowQuickFill;
   private Distance unitOfDistance;
   private int[ ]currencies;
   protected native void unpack( byte[ ]data);
   protected native byte[ ]pack( );
   public Pref( ) {
      super( null, new org.gnu.pdapilot.Char4( "exps"), 1, 1, true);
      }
   public Pref( byte[ ]contents, org.gnu.pdapilot.Char4 creator, int id, int version, boolean backup) {
      super( contents, creator, id, version, backup);
      }
   public String describe( ) {
      return super.describe( );
      /*return "syncType="+syncType+", getHigh=" +getHigh+", getContaining="+getContaining+", "+
			       "truncate="+truncate+", filterTo="+org.gnu.pdapilot.Util.prettyPrint(filterTo)+
			       ", filterFrom="+org.gnu.pdapilot.Util.prettyPrint(filterFrom)+
			       ", filterSubject="+org.gnu.pdapilot.Util.prettyPrint(filterSubject)+", "+super.describe();*/
      }
   // Accessor Methods
   public int getCurrentCategory( ) { return currentCategory; }
   public int getDefaultCategory( ) { return defaultCategory; }
   public int getNoteFont( ) { return noteFont; }
   public boolean getShowAllCategories( ) { return showAllCategories; }
   public boolean getShowCurrency( ) { return showCurrency; }
   public boolean getSaveBackup( ) { return saveBackup; }
   public boolean getAllowQuickFill( ) { return allowQuickFill; }
   public Distance getUnitOfDistrance( ) { return unitOfDistance; }
   public int[ ]getCurrencies( ) { return currencies; }
   public void setCurrentCategory( int c) { currentCategory = c; }
   public void setDefaultCategory( int d) { defaultCategory = d; }
   public void setNoteFont( int n) { noteFont = n; }
   public void setShowAllGategories( boolean s) { showAllCategories = s; }
   public void setShowCurrency( boolean s) { showCurrency = s; }
   public void setSaveBackup( boolean s) { saveBackup = s; }
   public void setAllowQuickFill( boolean a) { allowQuickFill = a; }
   public void setUnitOfDistance( Distance u) { unitOfDistance = u; }
   }
