/*
 * $Id: PdfString.java,v 1.26 2002/07/09 11:28:24 blowagie Exp $
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
 * Co-Developer of the code is Sid Steward. Portions created by the Co-Developer
 * are Copyright (C) 2010 by Sid Steward. All Rights Reserved.
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

package com.lowagie.text.pdf;
import java.io.OutputStream;
import java.io.IOException;

/**
 * A <CODE>PdfString</CODE>-class is the PDF-equivalent of a JAVA-<CODE>String</CODE>-object.
 * <P>
 * A string is a sequence of characters delimited by parenthesis. If a string is too long
 * to be conveniently placed on a single line, it may be split across multiple lines by using
 * the backslash character (\) at the end of a line to indicate that the string continues
 * on the following line. Within a string, the backslash character is used as an escape to
 * specify unbalanced parenthesis, non-printing ASCII characters, and the backslash character
 * itself. Use of the \<I>ddd</I> escape sequence is the preferred way to represent characters
 * outside the printable ASCII character set.<BR>
 * This object is described in the 'Portable Document Format Reference Manual version 1.3'
 * section 4.4 (page 37-39).
 *
 * @see		PdfObject
 * @see		BadPdfFormatException
 */

public class PdfString extends PdfObject {

    // ssteward: rewrite for pdftk 1.45
    // membervariables
    
    /** The value of this object. */
    protected String value= null; // NOTHING;
    //protected String originalValue = null; // TODO: this might be broken in pdftk 1.45
    protected byte[] originalBytes= null;
    
    /** The encoding. */
    // ssteward
    // the encoding indicates how to encode the data in the PDF file;
    // encodings:
    // null => not set
    // NOTHING => literal bytes; used with hex output
    // TEXT_UNICODE => UTF-16
    // TEXT_PDFDOCENCODING => PdfDoc encoding
    //
    protected String encoding= null;

    protected int objNum = 0;
    protected int objGen = 0;
    protected boolean hexWriting = false;

    // constructors
    
    /**
     * Constructs an empty <CODE>PdfString</CODE>-object.
     */
    
    public PdfString() {
        super( STRING );
    }
    
    /**
     * Constructs a <CODE>PdfString</CODE>-object.
     *
     * @param		value		the content of the string
     */
    
    // ssteward: rewrite for pdftk 1.45
    public PdfString( String value ) {
        super( STRING );
	this.value= value;
	this.bytes= getBytes();
    }
    
    /**
     * Constructs a <CODE>PdfString</CODE>-object.
     *
     * @param		value		the content of the string
     * @param		encoding	an encoding
     */
    
    // ssteward: rewrite for pdftk 1.45
    public PdfString( String value, String encoding ) {
        super( STRING );
        this.encoding= encoding;
        this.value= value;
	this.bytes= getBytes();
    }
    
    /**
     * Constructs a <CODE>PdfString</CODE>-object.
     *
     * @param		bytes	an array of <CODE>byte</CODE>
     */

    // construct from bytes, not value;
    // ssteward: rewrite for pdftk 1.45
    public PdfString( byte[] bytes, String encoding ) {
        super( STRING );
	this.encoding= encoding;
	this.bytes= bytes;
	this.value= getValue();
    }

    // ssteward: rewrite for pdftk 1.45
    public PdfString( byte[] bytes ) {
        super( STRING );
	this.bytes= bytes;
	this.value= getValue();
    }


    
    // methods overriding some methods in PdfObject
    
    /**
     * Returns the PDF representation of this <CODE>PdfString</CODE>.
     *
     * @return		an array of <CODE>byte</CODE>s
     */
    
    public void toPdf(PdfWriter writer, OutputStream os) throws IOException {
        byte b[] = getBytes();
        PdfEncryption crypto = null;
        if (writer != null)
            crypto = writer.getEncryption();
        if (crypto != null) {
            b = (byte[])bytes.clone();
            crypto.prepareKey();
            crypto.encryptRC4(b);
        }
        if (hexWriting) {
            ByteBuffer buf = new ByteBuffer();
            buf.append('<');
            int len = b.length;
            for (int k = 0; k < len; ++k)
                buf.appendHex(b[k]);
            buf.append('>');
            os.write(buf.toByteArray());
        }
        else
            os.write(PdfContentByte.escapeString(b));
    }
    
    /**
     * Returns the <CODE>String</CODE> value of the <CODE>PdfString</CODE>-object.
     *
     * @return		a <CODE>String</CODE>
     */
    public String toString() {
        return value;
    }
    
    // other methods
    
    /**
     * Gets the encoding of this string.
     *
     * @return		a <CODE>String</CODE>
     */
    
    public String getEncoding() {
        return encoding;
    }
    
    // ssteward: rewrite for pdftk 1.45
    public String toUnicodeString() {
	// value should be unicode
	/*
	//getBytes();
	if( bytes== null ) {
	    System.err.println( "bytes==null" );
	    if( encoding== NOTHING || encoding== null ) {
		System.err.println( "encoding=="+ encoding );
		if( PdfEncodings.isPdfDocEncoding(value) ) {
		    encoding= TEXT_PDFDOCENCODING;
		    System.err.println( "encoding==TEXT_PDFDOCENCODING "+ encoding  );
		}
		else {
		    encoding= TEXT_UNICODE;
		    System.err.println( "encoding==TEXT_UNICODE "+ encoding );
		}
	    }
	    bytes= PdfEncodings.convertToBytes( value, encoding );
	}
	*/
	//value= PdfEncodings.convertToString(bytes, PdfObject.TEXT_PDFDOCENCODING);

	//System.err.println( value );
	//System.err.println( encoding );
	return value;

	/*
	// if encoding is set, then the value is unicode, right? seems like flimsy logic;
	// let's see... encoding can only be set by the (String, encoding) constructor, so I guess
	// this works; it can also be set in getBytes(), which tries to detect a suitable
	// encoding; 
	if( encoding== null || encoding== NOTHING ) {
	    getBytes();
	    if (bytes.length >= 2 && bytes[0] == (byte)254 && bytes[1] == (byte)255) {

		// ssteward
		// pdftk-0.94: trim off these signature bytes (problem reading bookmarks);
		// pdftk-1.00: still an issue; problem reading EmbeddedFiles name tree keys,
		//    see PdfNameTree::iterateItems(); resulted in an extra null char at end of string
		byte b_copy[]= new byte[bytes.length- 2];
		System.arraycopy( bytes, 2, b_copy, 0, b_copy.length );

		//return PdfEncodings.convertToString(bytes, PdfObject.TEXT_UNICODE);
		value= PdfEncodings.convertToString(b_copy, PdfObject.TEXT_UNICODE);
	    }
	    else
		value= PdfEncodings.convertToString(bytes, PdfObject.TEXT_PDFDOCENCODING);
	}
	return value;
	*/
    }
    
    void setObjNum(int objNum, int objGen) {
        this.objNum = objNum;
        this.objGen = objGen;
    }
    
    // ssteward: rewrite for pdftk 1.45
    void decrypt(PdfReader reader) {
        PdfEncryption decrypt = reader.getDecrypt();
        if (decrypt != null) {
            //originalValue = value;
            decrypt.setHashKey(objNum, objGen);
            decrypt.prepareKey();
            //bytes = PdfEncodings.convertToBytes(value, null);
	    originalBytes= bytes;
            decrypt.encryptRC4(bytes);
            //value = PdfEncodings.convertToString(bytes, null);
	    value= getValue();
        }
    }
   
    // sets bytes and encoding based on value, if necessary
    // ssteward: rewrite for pdftk 1.45
    public byte[] getBytes() {
        if( bytes== null && value!= null ) {
	    if( encoding== null ) {
		if( PdfEncodings.isPdfDocEncoding(value) ) {
		    encoding= TEXT_PDFDOCENCODING;
		}
		else {
		    encoding= TEXT_UNICODE;
		}
	    }
	    bytes= PdfEncodings.convertToBytes( value, encoding );
        }
        return bytes;
    }

    // ssteward: added for pdftk 1.45
    private String getValue() {
	if( value== null && bytes!= null ) {
	    if( encoding== null ) {
		//if( bytes.length>= 2 && bytes[0]== (byte)254 && bytes[1]== (byte)255 ) {
		if( isUnicode( bytes ) ) {
		    encoding= TEXT_UNICODE;
		}
		else {
		    encoding= TEXT_PDFDOCENCODING;
		}
	    }
	    value= PdfEncodings.convertToString( bytes, encoding );
	}
	return this.value;
    }
    
    // ssteward: rewrite for pdftk 1.45
    public byte[] getOriginalBytes() {
        if( originalBytes!= null ) {
	    return originalBytes;
	}
	//return getBytes();
        //return PdfEncodings.convertToBytes(originalValue, null);
	return bytes;
    }
    
    public PdfString setHexWriting(boolean hexWriting) {
        this.hexWriting = hexWriting;
        return this;
    }
    
    public boolean isHexWriting() {
        return hexWriting;
    }

    /*
    public boolean isUnicode() {
	//getBytes();
	return( bytes.length >= 2 && bytes[0] == (byte)254 && bytes[1] == (byte)255 );
    }
    */

    // ssteward: rewrite for pdftk 1.45
    public static boolean isUnicode( byte[] bb ) {
	return( bb.length >= 2 && bb[0] == (byte)254 && bb[1] == (byte)255 );
    }
}
