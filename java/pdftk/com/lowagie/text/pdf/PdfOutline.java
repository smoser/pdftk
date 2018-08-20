/*
 * $Id: PdfOutline.java,v 1.51 2005/01/05 10:05:20 blowagie Exp $
 * $Name:  $
 *
 * Copyright 1999, 2000, 2001, 2002 Bruno Lowagie
 *
 *
 * The Original Code is 'iText, a free JAVA-PDF library'.
 *
 * The Initial Developer of the Original Code is Bruno Lowagie. Portions created by
 * the Initial Developer are Copyright (C) 1999, 2000, 2001, 2002 by Bruno Lowagie.
 * All Rights Reserved.
 * Co-Developer of the code is Paulo Soares. Portions created by the Co-Developer
 * are Copyright (C) 2000, 2001, 2002 by Paulo Soares. All Rights Reserved.
 *
 * Contributor(s): all the names of the contributors are added in the source code
 * where applicable.
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *
 * If you didn't download this code from the following link, you should check if
 * you aren't using an obsolete version:
 * http://www.lowagie.com/iText/
 */

package pdftk.com.lowagie.text.pdf;

import java.util.Iterator;

import pdftk.com.lowagie.text.Chunk;
import pdftk.com.lowagie.text.Paragraph;
import java.util.ArrayList;
import java.awt.Color;
import pdftk.com.lowagie.text.Font;
import java.io.OutputStream;
import java.io.IOException;

/**
 * <CODE>PdfOutline</CODE> is an object that represents a PDF outline entry.
 * <P>
 * An outline allows a user to access views of a document by name.<BR>
 * This object is described in the 'Portable Document Format Reference Manual version 1.3'
 * section 6.7 (page 104-106)
 *
 * @see		PdfDictionary
 */

public class PdfOutline extends PdfDictionary {
    
    // membervariables
    
    /** the <CODE>PdfIndirectReference</CODE> of this object */
    private PdfIndirectReference reference;
    
    /** value of the <B>Count</B>-key */
    private int count = 0;
    
    /** value of the <B>Parent</B>-key */
    private PdfOutline parent;
    
    /** value of the <B>Destination</B>-key */
    private PdfDestination destination;
    
    /** The <CODE>PdfAction</CODE> for this outline.
     */
    private PdfAction action;
       
    protected ArrayList kids = new ArrayList();
    
    protected PdfWriter writer;
    
    /** Holds value of property tag. */
    private String tag;
    
    /** Holds value of property open. */
    private boolean open;
    
    /** Holds value of property color. */
    private Color color;
    
    /** Holds value of property style. */
    private int style = 0;
    
    // constructors
    
    /**
     * Constructs a <CODE>PdfOutline</CODE>.
     * <P>
     * This is the constructor for the <CODE>outlines object</CODE>.
     * 
     * @param writer The PdfWriter you are adding the outline to
     */
    
    PdfOutline(PdfWriter writer) {
        super(OUTLINES);
        open = true;
        parent = null;
        this.writer = writer;
    }
    
    /**
     * Constructs a <CODE>PdfOutline</CODE>.
     * <P>
     * This is the constructor for an <CODE>outline entry</CODE>. The open mode is
     * <CODE>true</CODE>.
     *
     * @param parent the parent of this outline item
     * @param action the <CODE>PdfAction</CODE> for this outline item
     * @param title the title of this outline item
     */
    
    public PdfOutline(PdfOutline parent, PdfAction action, String title) {
        this(parent, action, title, true);
    }
    
    /**
     * Constructs a <CODE>PdfOutline</CODE>.
     * <P>
     * This is the constructor for an <CODE>outline entry</CODE>.
     *
     * @param parent the parent of this outline item
     * @param action the <CODE>PdfAction</CODE> for this outline item
     * @param title the title of this outline item
     * @param open <CODE>true</CODE> if the children are visible
     */
    public PdfOutline(PdfOutline parent, PdfAction action, String title, boolean open) {
        super();
        this.action = action;
        initOutline(parent, title, open);
    }
    
    /**
     * Constructs a <CODE>PdfOutline</CODE>.
     * <P>
     * This is the constructor for an <CODE>outline entry</CODE>. The open mode is
     * <CODE>true</CODE>.
     *
     * @param parent the parent of this outline item
     * @param destination the destination for this outline item
     * @param title the title of this outline item
     */
    
    public PdfOutline(PdfOutline parent, PdfDestination destination, String title) {
        this(parent, destination, title, true);
    }
    
    /**
     * Constructs a <CODE>PdfOutline</CODE>.
     * <P>
     * This is the constructor for an <CODE>outline entry</CODE>.
     *
     * @param parent the parent of this outline item
     * @param destination the destination for this outline item
     * @param title the title of this outline item
     * @param open <CODE>true</CODE> if the children are visible
     */
    public PdfOutline(PdfOutline parent, PdfDestination destination, String title, boolean open) {
        super();
        this.destination = destination;
        initOutline(parent, title, open);
    }
    
    /**
     * Constructs a <CODE>PdfOutline</CODE>.
     * <P>
     * This is the constructor for an <CODE>outline entry</CODE>. The open mode is
     * <CODE>true</CODE>.
     *
     * @param parent the parent of this outline item
     * @param action the <CODE>PdfAction</CODE> for this outline item
     * @param title the title of this outline item
     */
    public PdfOutline(PdfOutline parent, PdfAction action, PdfString title) {
        this(parent, action, title, true);
    }
    
    /**
     * Constructs a <CODE>PdfOutline</CODE>.
     * <P>
     * This is the constructor for an <CODE>outline entry</CODE>.
     *
     * @param parent the parent of this outline item
     * @param action the <CODE>PdfAction</CODE> for this outline item
     * @param title the title of this outline item
     * @param open <CODE>true</CODE> if the children are visible
     */
    public PdfOutline(PdfOutline parent, PdfAction action, PdfString title, boolean open) {
        this(parent, action, title.toString(), true);
    }
    
    /**
     * Constructs a <CODE>PdfOutline</CODE>.
     * <P>
     * This is the constructor for an <CODE>outline entry</CODE>. The open mode is
     * <CODE>true</CODE>.
     *
     * @param parent the parent of this outline item
     * @param destination the destination for this outline item
     * @param title the title of this outline item
     */
    
    public PdfOutline(PdfOutline parent, PdfDestination destination, PdfString title) {
        this(parent, destination, title, true);
    }
    
    /**
     * Constructs a <CODE>PdfOutline</CODE>.
     * <P>
     * This is the constructor for an <CODE>outline entry</CODE>.
     *
     * @param parent the parent of this outline item
     * @param destination the destination for this outline item
     * @param title the title of this outline item
     * @param open <CODE>true</CODE> if the children are visible
     */
    public PdfOutline(PdfOutline parent, PdfDestination destination, PdfString title, boolean open) {
        this(parent, destination, title.toString(), true);
    }
    
    /**
     * Constructs a <CODE>PdfOutline</CODE>.
     * <P>
     * This is the constructor for an <CODE>outline entry</CODE>. The open mode is
     * <CODE>true</CODE>.
     *
     * @param parent the parent of this outline item
     * @param action the <CODE>PdfAction</CODE> for this outline item
     * @param title the title of this outline item
     */
    
    public PdfOutline(PdfOutline parent, PdfAction action, Paragraph title) {
        this(parent, action, title, true);
    }
    
    /**
     * Constructs a <CODE>PdfOutline</CODE>.
     * <P>
     * This is the constructor for an <CODE>outline entry</CODE>.
     *
     * @param parent the parent of this outline item
     * @param action the <CODE>PdfAction</CODE> for this outline item
     * @param title the title of this outline item
     * @param open <CODE>true</CODE> if the children are visible
     */
    public PdfOutline(PdfOutline parent, PdfAction action, Paragraph title, boolean open) {
        super();
        StringBuffer buf = new StringBuffer();
        for (Iterator i = title.getChunks().iterator(); i.hasNext(); ) {
            Chunk chunk = (Chunk) i.next();
            buf.append(chunk.content());
        }
        this.action = action;
        initOutline(parent, buf.toString(), open);
    }
    
    /**
     * Constructs a <CODE>PdfOutline</CODE>.
     * <P>
     * This is the constructor for an <CODE>outline entry</CODE>. The open mode is
     * <CODE>true</CODE>.
     *
     * @param parent the parent of this outline item
     * @param destination the destination for this outline item
     * @param title the title of this outline item
     */
    
    public PdfOutline(PdfOutline parent, PdfDestination destination, Paragraph title) {
        this(parent, destination, title, true);
    }
    
    /**
     * Constructs a <CODE>PdfOutline</CODE>.
     * <P>
     * This is the constructor for an <CODE>outline entry</CODE>.
     *
     * @param parent the parent of this outline item
     * @param destination the destination for this outline item
     * @param title the title of this outline item
     * @param open <CODE>true</CODE> if the children are visible
     */
    public PdfOutline(PdfOutline parent, PdfDestination destination, Paragraph title, boolean open) {
        super();
        StringBuffer buf = new StringBuffer();
        for (Iterator i = title.getChunks().iterator(); i.hasNext(); ) {
            Chunk chunk = (Chunk) i.next();
            buf.append(chunk.content());
        }
        this.destination = destination;
        initOutline(parent, buf.toString(), open);
    }
    
    
    // methods
    
    /** Helper for the constructors.
     * @param parent the parent outline
     * @param title the title for this outline
     * @param open <CODE>true</CODE> if the children are visible
     */
    void initOutline(PdfOutline parent, String title, boolean open) {
        this.open = open;
        this.parent = parent;
        writer = parent.writer;
        put(PdfName.TITLE, new PdfString(title, PdfObject.TEXT_UNICODE));
        parent.addKid(this);
        if (destination != null && !destination.hasPage()) // bugfix Finn Bock
            setDestinationPage(writer.getCurrentPage());
    }
    
    /**
     * Sets the indirect reference of this <CODE>PdfOutline</CODE>.
     *
     * @param reference the <CODE>PdfIndirectReference</CODE> to this outline.
     */
    
    public void setIndirectReference(PdfIndirectReference reference) {
        this.reference = reference;
    }
    
    /**
     * Gets the indirect reference of this <CODE>PdfOutline</CODE>.
     *
     * @return		the <CODE>PdfIndirectReference</CODE> to this outline.
     */
    
    public PdfIndirectReference indirectReference() {
        return reference;
    }
    
    /**
     * Gets the parent of this <CODE>PdfOutline</CODE>.
     *
     * @return		the <CODE>PdfOutline</CODE> that is the parent of this outline.
     */
    
    public PdfOutline parent() {
        return parent;
    }
    
    /**
     * Set the page of the <CODE>PdfDestination</CODE>-object.
     *
     * @param pageReference indirect reference to the page
     * @return <CODE>true</CODE> if this page was set as the <CODE>PdfDestination</CODE>-page.
     */
    
    public boolean setDestinationPage(PdfIndirectReference pageReference) {
        if (destination == null) {
            return false;
        }
        return destination.addPage(pageReference);
    }
    
    /**
     * Gets the destination for this outline.
     * @return the destination
     */
    public PdfDestination getPdfDestination() {
        return destination;
    }
    
    int getCount() {
        return count;
    }

    void setCount(int count) {
        this.count = count;
    }
    
    /**
     * returns the level of this outline.
     *
     * @return		a level
     */
    
    public int level() {
        if (parent == null) {
            return 0;
        }
        return (parent.level() + 1);
    }
    
    /**
     * Returns the PDF representation of this <CODE>PdfOutline</CODE>.
     *
     * @param writer the encryption information
     * @param os
     * @throws IOException
     */
    
    public void toPdf(PdfWriter writer, OutputStream os) throws IOException {
        if (color != null && !color.equals(Color.black)) {
            put(PdfName.C, new PdfArray(new float[]{color.getRed()/255f,color.getGreen()/255f,color.getBlue()/255f}));
        }
        int flag = 0;
        if ((style & Font.BOLD) != 0)
            flag |= 2;
        if ((style & Font.ITALIC) != 0)
            flag |= 1;
        if (flag != 0)
            put(PdfName.F, new PdfNumber(flag));
        if (parent != null) {
            put(PdfName.PARENT, parent.indirectReference());
        }
        if (destination != null && destination.hasPage()) {
            put(PdfName.DEST, destination);
        }
        if (action != null)
            put(PdfName.A, action);
        if (count != 0) {
            put(PdfName.COUNT, new PdfNumber(count));
        }
        super.toPdf(writer, os);
    }
    
    /**
     * Adds a kid to the outline
     * @param outline
     */
    public void addKid(PdfOutline outline) {
        kids.add(outline);
    }
    
    /**
     * Returns the kids of this outline
     * @return an ArrayList with PdfOutlines
     */
    public ArrayList getKids() {
        return kids;
    }
    
    /**
     * Sets the kids of this outline
     * @param kids
     */
    public void setKids(ArrayList kids) {
        this.kids = kids;
    }
    
    /** Getter for property tag.
     * @return Value of property tag.
     */
    public String getTag() {
        return tag;
    }
    
    /** Setter for property tag.
     * @param tag New value of property tag.
     */
    public void setTag(String tag) {
        this.tag = tag;
    }
    
    /**
     * Gets the title of this outline
     * @return the title as a String
     */
    public String getTitle() {
        PdfString title = (PdfString)get(PdfName.TITLE);
        return title.toString();
    }
    
    /**
     * Sets the title of this outline
     * @param title
     */
    public void setTitle(String title) {
        put(PdfName.TITLE, new PdfString(title, PdfObject.TEXT_UNICODE));
    }
    
    /** Getter for property open.
     * @return Value of property open.
     */
    public boolean isOpen() {
        return open;
    }
    
    /** Setter for property open.
     * @param open New value of property open.
     */
    public void setOpen(boolean open) {
        this.open = open;
    }
    
    /** Getter for property color.
     * @return Value of property color.
     *
     */
    public Color getColor() {
        return this.color;
    }
    
    /** Setter for property color.
     * @param color New value of property color.
     *
     */
    public void setColor(Color color) {
        this.color = color;
    }
    
    /** Getter for property style.
     * @return Value of property style.
     *
     */
    public int getStyle() {
        return this.style;
    }
    
    /** Setter for property style.
     * @param style New value of property style.
     *
     */
    public void setStyle(int style) {
        this.style = style;
    }
    
}