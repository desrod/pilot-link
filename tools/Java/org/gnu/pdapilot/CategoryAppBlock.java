/* 
 * CategoryAppBlock.java:  Class describing AppBlocks.
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

package org.gnu.pdapilot;

import java.io.*;

public class CategoryAppBlock extends AppBlock {
   private boolean[] categoryRenamed;
   private int categoryLastUniqueID;
   private String[] categoryName;
   private int[] categoryID;
   public CategoryAppBlock() {
      super();
      }
   protected native void unpack(byte[] data);
   protected native byte[] pack();
   public void fill() {
      categoryRenamed = new boolean[16];
      categoryLastUniqueID = 0;
      categoryName = new String[16];
      categoryID = new int[16];
      }
   public CategoryAppBlock(byte[] contents) {
      super(contents);
      }
   public String describe() {
      StringBuffer c;
      if( ( categoryName == null) || (categoryID == null) || (categoryRenamed == null)) 
         {
            c = new StringBuffer("no categories");
            } 
         else {
            c = new StringBuffer("name             id\trenamed\n");
            for( int i=0; i < categoryName.length; i++) {
               if( categoryName[ i].length( ) > 0) 
                  {
                     c.append( categoryName[ i]);
                     for( int j=16; j > categoryName[ i].length( ); j--) c.append(" ");
                     c.append(" ");
                     c.append(categoryID[i]);
                     c.append("\t");
                     c.append(categoryRenamed[i]);
                     c.append("\n");
                     }
               }
            }
      return "categoryLastUniqueID=" + categoryLastUniqueID + "\n" + c;
      }
   // Accessor Methods
   public boolean[ ]getCategoryRenamed( ) { return categoryRenamed; }
   public int getCategoryLastUniqueID( ) { return categoryLastUniqueID; }
   public String[ ]getCategoryName( ) { return categoryName; }
   public int[ ]getCategoryID( ) { return categoryID; }
   public void setCategoryRenamed( boolean[ ]c) { categoryRenamed = c; }
   public void setCategoryLastUniqueID( int c) { categoryLastUniqueID = c; }
   public void setCategoryName( String[ ] c) { categoryName = c; }
   public void setCategoryID( int[ ]c) { categoryID = c; }
   }

		
