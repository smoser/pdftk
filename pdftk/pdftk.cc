/* -*- Mode: C++; tab-width: 2; c-basic-offset: 2 -*- */
/*
	pdftk, the PDF Tool Kit
	Copyright (c) 2003, 2004 Sid Steward

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	Visit: http://www.gnu.org/licenses/gpl.txt
	for more details on this license.

	Visit: http://www.pdftk.com for the latest information on pdftk

	Please contact Sid Steward with bug reports:
	ssteward at AccessPDF dot com
*/

// Tell C++ compiler to use Java-style exceptions.
#pragma GCC java_exceptions

#include <gcj/cni.h>

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <algorithm>

#include <java/lang/System.h>
#include <java/lang/Throwable.h>
#include <java/lang/String.h>
#include <java/io/IOException.h>
#include <java/io/PrintStream.h>
#include <java/io/FileOutputStream.h>
#include <java/util/Vector.h>
#include <java/util/ArrayList.h>
#include <java/util/Set.h>
#include <java/util/Iterator.h>

#include "com/lowagie/text/Document.h"
#include "com/lowagie/text/pdf/PdfName.h"
#include "com/lowagie/text/pdf/PdfString.h"
#include "com/lowagie/text/pdf/PdfNumber.h"
#include "com/lowagie/text/pdf/PdfArray.h"
#include "com/lowagie/text/pdf/PdfDictionary.h"
#include "com/lowagie/text/pdf/PdfOutline.h"
#include "com/lowagie/text/pdf/PdfCopy.h"
#include "com/lowagie/text/pdf/PdfReader.h"
#include "com/lowagie/text/pdf/PdfImportedPage.h"
#include "com/lowagie/text/pdf/PdfStamper.h"
#include "com/lowagie/text/pdf/PdfStamperImp.h"
#include "com/lowagie/text/pdf/PdfEncryptor.h"

using namespace std;

namespace java {
	using namespace java::lang;
	using namespace java::io;
	using namespace java::util;
}

namespace itext {
	using namespace com::lowagie::text;
	using namespace com::lowagie::text::pdf;
}

// store java::PdfReader* here to 
// prevent unwanted garbage collection
//
java::Vector* g_dont_collect_p= 0;


static void
describe_header();

static void
describe_usage();


class TK_Session {
	
	bool m_valid_b;
	bool m_authorized_b;
	bool m_input_pdf_readers_opened_b; // have m_input_pdf readers been opened?
	bool m_verbose_reporting_b;

public:

	struct InputPdf {
		string m_filename;
		string m_password;
		bool m_authorized_b;

		// keep track of which pages get output under which readers,
		// because one reader mayn't output the same page twice;
		vector< pair< set<jint>, itext::PdfReader* > > m_readers;

		jint m_num_pages;

		InputPdf() : m_authorized_b(true), m_num_pages(0) {}
	};
	// pack input PDF in the order their given on the command line
	vector< InputPdf > m_input_pdf;
	typedef vector< InputPdf >::size_type InputPdfIndex;

	// store input PDF handles here
	map< string, InputPdfIndex > m_input_pdf_index;

	bool add_reader( InputPdf* input_pdf_p );
	bool open_input_pdf_readers();

  enum keyword {
    none_k= 0,

    // the operations
    cat_k, // output may also be encrypted
		burst_k, // split a single, input PDF into individual pages
		filter_k, // apply 'filters' to a single, input PDF based on output args
		dump_data_k, // no PDF output
		//
		first_operation_k= cat_k,
    final_operation_k= dump_data_k,

		// cat page range keywords
    end_k,
    even_k,
    odd_k,

    output_k,

		// encryption & decryption
		input_pw_k,
		owner_pw_k,
		user_pw_k,
		user_perms_k,

		// output arg.s, only
		encrypt_40bit_k,
		encrypt_128bit_k,

		// user permissions
		perm_printing_k,
		perm_modify_contents_k,
		perm_copy_contents_k,
		perm_modify_annotations_k,
		perm_fillin_k,
		perm_screen_readers_k,
		perm_assembly_k,
		perm_degraded_printing_k,
		perm_all_k,

		// filters
		filt_uncompress_k,
		filt_compress_k,

		// pdftk options
		verbose_k
  };
  static keyword is_keyword( char* ss, int* keyword_len_p );

  keyword m_operation;

  typedef unsigned long PageNumber;

  struct PageRef {
		InputPdfIndex m_input_pdf_index;
    PageNumber m_page_num; // 1-based

		PageRef( InputPdfIndex input_pdf_index, PageNumber page_num ) :
			m_input_pdf_index( input_pdf_index ), m_page_num( page_num ) {}			
  };
  vector< PageRef > m_page_seq;

  string m_output_filename;
	string m_output_owner_pw;
	string m_output_user_pw;
	jint m_output_user_perms;
	bool m_output_uncompress_b;
	bool m_output_compress_b;

	enum encryption_strength {
		none_enc= 0,
		bits40_enc,
		bits128_enc
	} m_output_encryption_strength;

  TK_Session( int argc, 
							char** argv );

	~TK_Session();

  bool is_valid() const;

	void dump_session_data() const;

	void create_output();

};

static void 
OutputString( ostream& ofs,
							java::lang::String* jss_p );

bool
TK_Session::add_reader( InputPdf* input_pdf_p )
{
	bool open_success_b= true;

	try {
		itext::PdfReader* reader= 0;
		if( input_pdf_p->m_password.empty() ) {
			reader=
				new itext::PdfReader( JvNewStringLatin1( input_pdf_p->m_filename.c_str() ) );
		}
		else {
			jbyteArray password= JvNewByteArray( input_pdf_p->m_password.size() );
			memcpy( (char*)(elements(password)), 
							input_pdf_p->m_password.c_str(),
							input_pdf_p->m_password.size() );

			reader= 
				new itext::PdfReader( JvNewStringLatin1( input_pdf_p->m_filename.c_str() ),
															password );
		}
		reader->consolidateNamedDestinations();
		reader->removeUnusedObjects();

		input_pdf_p->m_num_pages= reader->getNumberOfPages();

		// keep tally of which pages have been laid claim to in this reader;
		// when creating the final PDF, this tally will be decremented
		input_pdf_p->m_readers.push_back( pair< set<jint>, itext::PdfReader* >( set<jint>(), reader ) );
		
							
		// store in this java object so the gc can trace it
		g_dont_collect_p->addElement( reader );

		input_pdf_p->m_authorized_b= ( !reader->encrypted || reader->passwordIsOwner );
		if( !input_pdf_p->m_authorized_b ) {
			open_success_b= false;
		}
	}
	catch( java::io::IOException* ioe_p ) { // file open error

		if( ioe_p->getMessage()->equals( JvNewStringLatin1( "Bad password" ) ) ) {
			input_pdf_p->m_authorized_b= false;
		}
		open_success_b= false;
	}
	catch( java::lang::Throwable* t_p ) { // unexpected error
		cerr << "Error: Unexpected Exception in open_reader()" << endl;
		open_success_b= false;
							
		//t_p->printStackTrace(); // debug
	}

	// report
	if( !open_success_b ) { // file open error
		cerr << "Error: Failed to open PDF file: " << endl;
		cerr << "   " << input_pdf_p->m_filename << endl;
		if( !input_pdf_p->m_authorized_b ) {
			cerr << "   OWNER PASSWORD REQUIRED, but not given (or incorrect)" << endl;
		}
	}

	// update session state
	m_authorized_b= m_authorized_b && input_pdf_p->m_authorized_b;

	return open_success_b;
}

bool 
TK_Session::open_input_pdf_readers()
{
	// try opening the input files and init m_input_pdf readers
	bool open_success_b= true;

	if( !m_input_pdf_readers_opened_b ) {
		for( vector< InputPdf >::iterator it= m_input_pdf.begin(); it!= m_input_pdf.end(); ++it ) {
			open_success_b= add_reader( &(*it) ) && open_success_b;
		}
		m_input_pdf_readers_opened_b= open_success_b;
	}

	return open_success_b;
}

static int
copy_downcase( char* ll, int ll_len,
							 char* rr )
{
  int ii= 0;
  for( ; rr[ii] && ii< ll_len- 1; ++ii ) {
    ll[ii]= 
      ( 'A'<= rr[ii] && rr[ii]<= 'Z' ) ? 
      rr[ii]- ('A'- 'a') : 
      rr[ii];
  }

  ll[ii]= 0;

	return ii;
}

TK_Session::keyword
TK_Session::is_keyword( char* ss, int* keyword_len_p )
{
  *keyword_len_p= 0;

  const int ss_copy_max= 256;
  char ss_copy[ss_copy_max]= "";
  int ss_copy_size= copy_downcase( ss_copy, ss_copy_max, ss );

	*keyword_len_p= ss_copy_size; // possibly modified, below

	// operations
  if( strcmp( ss_copy, "cat" )== 0 ) {
    return cat_k;
  }
	else if( strcmp( ss_copy, "burst" )== 0 ) {
		return burst_k;
	}
	else if( strcmp( ss_copy, "filter" )== 0 ) {
		return filter_k;
	}
	else if( strcmp( ss_copy, "dump_data" )== 0 ||
					 strcmp( ss_copy, "dumpdata" )== 0 ||
					 strcmp( ss_copy, "data_dump" )== 0 ||
					 strcmp( ss_copy, "datadump" )== 0 ) {
		return dump_data_k;
	}

	// cat range keywords
  else if( strncmp( ss_copy, "end", 3 )== 0 ) { // note: strncmp
    *keyword_len_p= 3; // note: fixed size
    return end_k;
  }
  else if( strcmp( ss_copy, "even" )== 0 ) {
    return even_k;
  }
  else if( strcmp( ss_copy, "odd" )== 0 ) {
    return odd_k;
  }

  else if( strcmp( ss_copy, "output" )== 0 ) {
    return output_k;
  }

	// encryption & decryption; depends on context
	else if( strcmp( ss_copy, "owner_pw" )== 0 ||
					 strcmp( ss_copy, "ownerpw" )== 0 ) {
		return owner_pw_k;
	}
	else if( strcmp( ss_copy, "user_pw" )== 0 ||
					 strcmp( ss_copy, "userpw" )== 0 ) {
		return user_pw_k;
	}
	else if( strcmp( ss_copy, "input_pw" )== 0 ||
					 strcmp( ss_copy, "inputpw" )== 0 ) {
		return input_pw_k;
	}
	else if( strcmp( ss_copy, "allow" )== 0 ) {
		return user_perms_k;
	}

	// expect these only in output section
	else if( strcmp( ss_copy, "encrypt_40bit" )== 0 ||
					 strcmp( ss_copy, "encrypt_40bits" )== 0 ||
					 strcmp( ss_copy, "encrypt40bit" )== 0 ||
					 strcmp( ss_copy, "encrypt40bits" )== 0 ||
					 strcmp( ss_copy, "encrypt40_bit" )== 0 ||
					 strcmp( ss_copy, "encrypt40_bits" )== 0 ||
					 strcmp( ss_copy, "encrypt_40_bit" )== 0 ||
					 strcmp( ss_copy, "encrypt_40_bits" )== 0 ) {
		return encrypt_40bit_k;
	}
	else if( strcmp( ss_copy, "encrypt_128bit" )== 0 ||
					 strcmp( ss_copy, "encrypt_128bits" )== 0 ||
					 strcmp( ss_copy, "encrypt128bit" )== 0 ||
					 strcmp( ss_copy, "encrypt128bits" )== 0 ||
					 strcmp( ss_copy, "encrypt128_bit" )== 0 ||
					 strcmp( ss_copy, "encrypt128_bits" )== 0 ||
					 strcmp( ss_copy, "encrypt_128_bit" )== 0 ||
					 strcmp( ss_copy, "encrypt_128_bits" )== 0 ) {
		return encrypt_128bit_k;
	}
	
	// user permissions; must follow user_perms_k;
	else if( strcmp( ss_copy, "printing" )== 0 ) {
		return perm_printing_k;
	}
	else if( strcmp( ss_copy, "modifycontents" )== 0 ) {
		return perm_modify_contents_k;
	}
	else if( strcmp( ss_copy, "copycontents" )== 0 ) {
		return perm_copy_contents_k;
	}
	else if( strcmp( ss_copy, "modifyannotations" )== 0 ) {
		return perm_modify_annotations_k;
	}
	else if( strcmp( ss_copy, "fillin" )== 0 ) {
		return perm_fillin_k;
	}
	else if( strcmp( ss_copy, "screenreaders" )== 0 ) {
		return perm_screen_readers_k;
	}
	else if( strcmp( ss_copy, "assembly" )== 0 ) {
		return perm_assembly_k;
	}
	else if( strcmp( ss_copy, "degradedprinting" )== 0 ) {
		return perm_degraded_printing_k;
	}
	else if( strcmp( ss_copy, "allfeatures" )== 0 ) {
		return perm_all_k;
	}
	else if( strcmp( ss_copy, "uncompress" )== 0 ) {
		return filt_uncompress_k;
	}
	else if( strcmp( ss_copy, "compress" )== 0 ) {
		return filt_compress_k;
	}
	else if( strcmp( ss_copy, "verbose" )== 0 ) {
		return verbose_k;
	}
	
  return none_k;
}

bool
TK_Session::is_valid() const
{
	return( m_valid_b &&
					( m_operation== dump_data_k || m_authorized_b ) &&
					!m_input_pdf.empty() &&
					m_input_pdf_readers_opened_b &&

					first_operation_k<= m_operation &&
					m_operation<= final_operation_k &&

					// these op.s require a single input PDF file
					( !( m_operation== burst_k ||
							 m_operation== filter_k ) ||
						( m_input_pdf.size()== 1 ) ) &&

					// these op.s do not require an output filename
					( m_operation== burst_k ||
					  m_operation== dump_data_k ||
					  !m_output_filename.empty() ) );
}

void
TK_Session::dump_session_data() const
{
	if( !m_verbose_reporting_b )
		return;

	if( !m_input_pdf_readers_opened_b ) {
		cout << "Input PDF Open Errors" << endl;
		return;
	}

	//
	if( is_valid() ) {
		cout << "Command Line Data is valid." << endl;
	}
	else { 
		cout << "Command Line Data is NOT valid." << endl;
	}

	// input files
	cout << endl;
	cout << "Input PDF Filenames & Passwords in Order\n( <filename>[, <password>] ) " << endl;
	if( m_input_pdf.empty() ) {
		cout << "   No input PDF filenames have been given." << endl;
	}
	else {
		for( vector< InputPdf >::const_iterator it= m_input_pdf.begin();
				 it!= m_input_pdf.end(); ++it )
			{
				cout << "   " << it->m_filename;
				if( !it->m_password.empty() ) {
					cout << ", " << it->m_password;
				}

				if( !it->m_authorized_b ) {
					cout << ", OWNER PASSWORD REQUIRED, but not given (or incorrect)";
				}

				cout << endl;
			}
	}

	// operation
	cout << endl;
	cout << "The operation to be performed: " << endl;
	switch( m_operation ) {
	case cat_k:
		cout << "   cat - Catenate given page ranges into a new PDF." << endl;
		break;
	case burst_k:
		cout << "   burst - Split a single, input PDF into individual pages." << endl;
		break;
	case filter_k:
		cout << "   filter - Apply 'filters' to a single, input PDF based on output args." << endl;
		cout << "      (When the operation is omitted, this is the default.)" << endl;
		break;
	case dump_data_k:
		cout << "   dump_data - Report statistics on a single, input PDF." << endl;
		break;
	case none_k:
		cout << "   NONE - No operation has been given.  See usage instructions." << endl;
		break;
	default:
		cout << "   INTERNAL ERROR - An unexpected operation has been given." << endl;
		break;
	}

	// pages
	/*
	cout << endl;
	cout << "The following pages will be operated on, in the given order." << endl;
	if( m_page_seq.empty() ) {
		cout << "   No pages or page ranges have been given." << endl;
	}
	else {
		for( vector< PageRef >::const_iterator it= m_page_seq.begin();
				 it!= m_page_seq.end(); ++it )
			{
				map< string, InputPdf >::const_iterator jt=
					m_input_pdf.find( it->m_handle );
				if( jt!= m_input_pdf.end() ) {
					cout << "   Handle: " << it->m_handle << "  File: " << jt->second.m_filename;
					cout << "  Page: " << it->m_page_num << endl;
				}
				else { // error
					cout << "   Internal Error: handle not found in m_input_pdf: " << it->m_handle << endl;
				}
			}
	}
	*/

	// output file; may be PDF or text
	cout << endl;
	cout << "The output file will be named:" << endl;
	if( m_output_filename.empty() ) {
		cout << "   No output filename has been given." << endl;
	}
	else {
		cout << "   " << m_output_filename << endl;
	}

	// output encryption
	cout << endl;
	bool output_encrypted_b= 
		m_output_encryption_strength!= none_enc ||
		!m_output_user_pw.empty() ||
		!m_output_owner_pw.empty();

	cout << "Output PDF encryption settings:" << endl;
	if( output_encrypted_b ) {
		cout << "   Output PDF will be encrypted." << endl;

		switch( m_output_encryption_strength ) {
		case none_enc:
			cout << "   Encryption strength not given. Defaulting to: 128 bits." << endl;
			break;
		case bits40_enc:
			cout << "   Given output encryption strength: 40 bits" << endl;
			break;
		case bits128_enc:
			cout << "   Given output encryption strength: 128 bits" << endl;
			break;
		}

		cout << endl;
		{
			using com::lowagie::text::pdf::PdfWriter;

			if( m_output_user_pw.empty() )
				cout << "   No user password given." << endl;
			else
				cout << "   Given user password: " << m_output_user_pw << endl;
			if( m_output_owner_pw.empty() )
				cout << "   No owner password given." << endl;
			else
				cout << "   Given owner password: " << m_output_owner_pw << endl;
			//
			// the printing section: Top Quality or Degraded, but not both;
			// AllowPrinting is a superset of both flag settings
			if( (m_output_user_perms & PdfWriter::AllowPrinting)== PdfWriter::AllowPrinting )
				cout << "   ALLOW Top Quality Printing" << endl;
			else if( (m_output_user_perms & PdfWriter::AllowPrinting)== PdfWriter::AllowDegradedPrinting )
				cout << "   ALLOW Degraded Printing (Top-Quality Printing NOT Allowed)" << endl;
			else
				cout << "   Printing NOT Allowed" << endl;
			if( (m_output_user_perms & PdfWriter::AllowModifyContents)== PdfWriter::AllowModifyContents )
				cout << "   ALLOW Modifying of Contents" << endl;
			else
				cout << "   Modifying of Contents NOT Allowed" << endl;
			if( (m_output_user_perms & PdfWriter::AllowCopy)== PdfWriter::AllowCopy )
				cout << "   ALLOW Copying of Contents" << endl;
			else
				cout << "   Copying of Contents NOT Allowed" << endl;
			if( (m_output_user_perms & PdfWriter::AllowModifyAnnotations)== PdfWriter::AllowModifyAnnotations )
				cout << "   ALLOW Modifying of Annotations" << endl;
			else
				cout << "   Modifying of Annotations NOT Allowed" << endl;
			if( (m_output_user_perms & PdfWriter::AllowFillIn)== PdfWriter::AllowFillIn )
				cout << "   ALLOW Fill-In" << endl;
			else
				cout << "   Fill-In NOT Allowed" << endl;
			if( (m_output_user_perms & PdfWriter::AllowScreenReaders)== PdfWriter::AllowScreenReaders )
				cout << "   ALLOW Screen Readers" << endl;
			else
				cout << "   Screen Readers NOT Allowed" << endl;
			if( (m_output_user_perms & PdfWriter::AllowAssembly)== PdfWriter::AllowAssembly )
				cout << "   ALLOW Assembly" << endl;
			else
				cout << "   Assembly NOT Allowed" << endl;
		}
	}
	else {
		cout << "   Output PDF will not be encrypted." << endl;
	}

	// compression filter
	cout << endl;
	if( m_operation!= filter_k ||
			output_encrypted_b ||
			!( m_output_compress_b ||
				 m_output_uncompress_b ) )
		{
			cout << "No compression or uncompression being performed on output." << endl;
		}
	else {
		if( m_output_compress_b ) {
			cout << "Compression will be applied to some PDF streams." << endl;
		}
		else {
			cout << "Some PDF streams will be uncompressed." << endl;
		}
	}

}

TK_Session::TK_Session( int argc, 
												char** argv ) :
	m_valid_b( false ),
	m_authorized_b( true ),
	m_input_pdf_readers_opened_b( false ),
  m_operation( none_k ),
	m_output_user_perms( 0 ),
	m_output_uncompress_b( false ),
	m_output_compress_b( false ),
	m_output_encryption_strength( none_enc ),
	m_verbose_reporting_b( false )
{
  enum {
    input_files_e,
		input_pw_e,

    operation_e,
    page_seq_e,

    output_filename_e,

		output_args_e, // output args are order-independent; switch here
		output_owner_pw_e,
		output_user_pw_e,
		output_user_perms_e,

		done_e
  } arg_state = input_files_e;

	g_dont_collect_p= new java::Vector();

  bool password_using_handles_not_b= false;
  bool password_using_handles_b= false;
	InputPdfIndex password_input_pdf_index= 0;

  bool fail_b= false;

  // iterate over cmd line arguments
  for( int ii= 1; ii< argc && !fail_b && arg_state!= done_e; ++ii ) {
    int keyword_len= 0;
    keyword arg_keyword= is_keyword( argv[ii], &keyword_len );

    switch( arg_state ) {
    case input_files_e: 
			// input_files_e:
      // expecting input handle=filename pairs, or
      // an input filename w/o a handle, or
			// the PDF passwords keyword, or
      // an operation keyword, or
			// the output keyword

		case input_pw_e: {
			// input_pw_e:
      // expecting input handle=password pairs, or
      // an input PDF password w/o a handle, or
      // an operation keyword, or
			// the output keyword

			if( arg_keyword== input_pw_k ) { // input PDF passwords keyword

				arg_state= input_pw_e;
			}
      else if( arg_keyword== cat_k ) {
				m_operation= cat_k;
				arg_state= page_seq_e; // collect page sequeces
      }
      else if( arg_keyword== burst_k ) {
				m_operation= burst_k;
				arg_state= output_args_e; // skip "output <fn>" bit
      }
			else if( arg_keyword== filter_k ) {
				m_operation= filter_k;
				arg_state= page_seq_e;
			}
			else if( arg_keyword== dump_data_k ) {
				m_operation= dump_data_k;
				arg_state= page_seq_e; // look for an output filename
			}
			else if( arg_keyword== output_k ) { // we reached the output section
				arg_state= output_filename_e;
			}
      else if( arg_keyword== none_k ) {
				// here is where the two cases (input_files_e, input_pw_e) diverge

				if( arg_state== input_files_e ) {
					// treat argv[ii] like an optional input handle and filename
					// like this: [<handle>=]<filename>

					char* eq_loc= strchr( argv[ii], '=' );

					if( eq_loc== 0 ) { // no equal sign; no handle

							InputPdf input_pdf;
							input_pdf.m_filename= argv[ii];
							m_input_pdf.push_back( input_pdf );
					}
					else { // use given handle for filename; test, first
						
						if( ( argv[ii]+ 1< eq_loc ) ||
								!( 'A'<= argv[ii][0] && argv[ii][0]<= 'Z' ) ) 
							{ // error
								cerr << "Error: Handle can only be a single, upper-case letter" << endl;
								cerr << "   here: " << argv[ii] << " Exiting." << endl;
								fail_b= true;
							}
						else {
							// look up handle
							map< string, InputPdfIndex >::const_iterator it= 
								m_input_pdf_index.find( string(1, argv[ii][0]) );

							if( it!= m_input_pdf_index.end() ) { // error: alreay in use
								cerr << "Error: Handle given here: " << endl;
								cerr << "      " << argv[ii] << endl;
								cerr << "   is already associated with: " << endl;
								cerr << "      " << m_input_pdf[it->second].m_filename << endl;
								cerr << "   Exiting." << endl;
								fail_b= true;
							}
							else { // add handle/filename association
								*eq_loc= 0;

								InputPdf input_pdf;
								input_pdf.m_filename= eq_loc+ 1;
								m_input_pdf.push_back( input_pdf );

								m_input_pdf_index[ string(1, argv[ii][0]) ]= m_input_pdf.size()- 1;
							}
						}
					}
				} // end: arg_state== input_files_e
				else { // arg_state== input_pw_e

					// treat argv[ii] like an input handle and password
					// like this <handle>=<password>; if no handle is
					// given, assign passwords to input in order;

					char* eq_loc= strchr( argv[ii], '=' );

					if( eq_loc== 0 ) { // no equal sign; try using default handles
						if( password_using_handles_b ) { // error: expected a handle
							cerr << "Error: Expected a user-supplied handle for this input" << endl;
							cerr << "   PDF password: " << argv[ii] << endl << endl;
							cerr << "   Handles must be supplied with ~all~ input" << endl;
							cerr << "   PDF passwords, or with ~no~ input PDF passwords." << endl;
							cerr << "   If no handles are supplied, then passwords are applied" << endl;
							cerr << "   according to input PDF order." << endl << endl;
							cerr << "   Handles are given like this: <handle>=<password>, and" << endl;
							cerr << "   they must be single, upper case letters, like: A, B, etc." << endl;
							fail_b= true;
						}
						else {
							password_using_handles_not_b= true;

							if( password_input_pdf_index< m_input_pdf.size() ) {
								m_input_pdf[password_input_pdf_index++].m_password= argv[ii]; // set
							}
							else { // error
								cerr << "Error: more input passwords than input PDF documents." << endl;
								cerr << "   Exiting." << endl;
								fail_b= true;
							}
						}
					}
					else { // use given handle for password
						if( password_using_handles_not_b ) { // error; remark and set fail_b
							cerr << "Error: Expected ~no~ user-supplied handle for this input" << endl;
							cerr << "   PDF password: " << argv[ii] << endl << endl;
							cerr << "   Handles must be supplied with ~all~ input" << endl;
							cerr << "   PDF passwords, or with ~no~ input PDF passwords." << endl;
							cerr << "   If no handles are supplied, then passwords are applied" << endl;
							cerr << "   according to input PDF order." << endl << endl;
							cerr << "   Handles are given like this: <handle>=<password>, and" << endl;
							cerr << "   they must be single, upper case letters, like: A, B, etc." << endl;
							fail_b= true;
						}
						else if( argv[ii]+ 0== eq_loc ) { // error
							cerr << "Error: No user-supplied handle found" << endl;
							cerr << "   at: " << argv[ii] << " Exiting." << endl;
							fail_b= true;
						}
						else if( ( argv[ii]+ 1< eq_loc ) ||
										 !( 'A'<= argv[ii][0] && argv[ii][0]<= 'Z' ) ) 
							{ // error
								cerr << "Error: Handle can only be a single, upper-case letter" << endl;
								cerr << "   here: " << argv[ii] << " Exiting." << endl;
								fail_b= true;
							}
						else {
							password_using_handles_b= true;

							// look up this handle
							map< string, InputPdfIndex >::const_iterator it= 
								m_input_pdf_index.find( string(1, argv[ii][0]) );
							if( it!= m_input_pdf_index.end() ) { // found

								if( m_input_pdf[it->second].m_password.empty() ) {
									m_input_pdf[it->second].m_password= eq_loc+ 1; // set
								}
								else { // error: password already given
									cerr << "Error: Handle given here: " << endl;
									cerr << "      " << argv[ii] << endl;
									cerr << "   is already associated with this password: " << endl;
									cerr << "      " << m_input_pdf[it->second].m_password << endl;
									cerr << "   Exiting." << endl;
									fail_b= true;
								}
							}
							else { // error: no input file matches this handle
								cerr << "Error: Password handle: " << argv[ii] << endl;
								cerr << "   is not associated with an input PDF file." << endl;
								cerr << "   Exiting." << endl;
								fail_b= true;
							}
						}
					}
				} // end: arg_state== input_pw_e
			}
			else { // error: unexpected keyword; remark and set fail_b
				cerr << "Error: Unexpected command-line data: " << endl;
				cerr << "      " << argv[ii] << endl;
				if( arg_state== input_files_e ) {
					cerr << "   where we were expecting an input PDF filename," << endl;
					cerr << "   operation (e.g. \"cat\") or \"input_pw\".  Exiting." << endl;
				}
				else {
					cerr << "   where we were expecting an input PDF password" << endl;
					cerr << "   or operation (e.g. \"cat\").  Exiting." << endl;
				}
				fail_b= true;
			}
    }
    break;

    case operation_e: {
			// error; this state should always be skipped
			cerr << "Internal Error: Unexpected case: operation_k.  Exiting." << endl;
		}
		break;

    case page_seq_e: {
      if( m_page_seq.empty() ) {
				// we just got here; validate input filenames

				if( m_input_pdf.empty() ) { // error; remark and set fail_b
					cerr << "Error: No input files.  Exiting." << endl;
					fail_b= true;
					break;
				}

				// try opening input PDF readers
				if( !open_input_pdf_readers() ) { // failure
					fail_b= true;
					break;
				}
      } // end: first pass init. pdf files

      if( arg_keyword== output_k ) {
				arg_state= output_filename_e; // advance state
				break;
      }
      else if( arg_keyword== none_k || 
							 arg_keyword== end_k )
				{ // treat argv[ii] like a page sequence

					bool even_pages_b= false;
					bool odd_pages_b= false;
					int jj= 0;

					InputPdfIndex range_pdf_index= 0; { // defaults to first input document
						string handle;
						// in anticipation of multi-character handles (?)
						for( ; argv[ii][jj] && isupper(argv[ii][jj]); ++jj ) {
							handle.push_back( argv[ii][jj] );
						}
						if( !handle.empty() ) {
							// validate handle
							map< string, InputPdfIndex >::const_iterator it= m_input_pdf_index.find( handle );
							if( it== m_input_pdf_index.end() ) { // error
								cerr << "Error: Given handle has no associated file: " << endl;
								cerr << "   " << handle << ", used here: " << argv[ii] << endl;
								cerr << "   Exiting." << endl;
								fail_b= true;
								break;
							}
							else {
								range_pdf_index= it->second;
							}
						}
					}

					char* hyphen_loc= strchr( argv[ii]+ jj, '-' );
					if( hyphen_loc )
						*hyphen_loc= 0;

					////
					// start of page range

					PageNumber page_num_beg= 0;
					for( ; argv[ii][jj] && isdigit(argv[ii][jj]); ++jj ) {
						page_num_beg= page_num_beg* 10+ argv[ii][jj]- '0';
					}

					if( argv[ii][jj] ) { // process possible text keyword in page range start
						if( page_num_beg ) { // error: can't have numbers ~and~ a keyword at the beginning
							cerr << "Error: Unexpected combination of digits and text in" << endl;
							cerr << "   page range start, here: " << argv[ii] << endl;
							cerr << "   Exiting." << endl;
							fail_b= true;
							break;
						}
					
						// read keyword
						int keyword_len= 0;
						keyword arg_keyword= is_keyword( argv[ii]+ jj, &keyword_len );

						if( arg_keyword== end_k ) {
							page_num_beg= m_input_pdf[range_pdf_index].m_num_pages;
						}
						else if( !hyphen_loc ) { // no end of page range given

							// even and odd keywords could be used when referencing
							// an entire document by handle, e.g. Aeven, Aodd
							//
							if( arg_keyword== even_k ) {
								even_pages_b= true;
							}
							else if( arg_keyword== odd_k ) {
								odd_pages_b= true;
							}
							else { // error; unexpected keyword or string
								cerr << "Error: Unexpected text in page reference, here: " << endl;
								cerr << "   " << argv[ii] << endl;
								cerr << "   Exiting." << endl;
								cerr << "   Acceptable keywords, here, are: \"even\", \"odd\", or \"end\"." << endl;
								fail_b= true;
								break;
							}
						}
						else { // error; unexpected keyword or string
							cerr << "Error: Unexpected letters in page range start, here: " << endl;
							cerr << "   " << argv[ii] << endl;
							cerr << "   Exiting." << endl;
							cerr << "   The acceptable keyword, here, is \"end\"." << endl;
							fail_b= true;
							break;
						}

						jj+= keyword_len;
					}
	
					// advance to end of token
					while( argv[ii][jj] ) { 
						++jj;
					}

					////
					// end of page range

					PageNumber page_num_end= 0;

					if( hyphen_loc ) { // process second half of page range
						++jj; // jump over hyphen

						// digits
						for( ; argv[ii][jj] && isdigit(argv[ii][jj]); ++jj ) {
							page_num_end= page_num_end* 10+ argv[ii][jj]- '0';
						}

						// trailing text
						while( argv[ii][jj] ) {

							// read keyword
							int keyword_len= 0;
							keyword arg_keyword= is_keyword( argv[ii]+ jj, &keyword_len );

							if( page_num_end ) {
								if( arg_keyword== even_k ) {
									even_pages_b= true;
								}
								else if( arg_keyword== odd_k ) {
									odd_pages_b= true;
								}
								else { // error
									cerr << "Error: Unexpected text in page range end, here: " << endl;
									cerr << "   " << argv[ii] << endl;
									cerr << "   Exiting." << endl;
									cerr << "   Acceptable keywords, here, are: \"even\" or \"odd\"." << endl;
									fail_b= true;
									break;
								}
							}
							else { // !page_num_end
								if( arg_keyword== end_k ) {
									page_num_end= m_input_pdf[range_pdf_index].m_num_pages;
								}
								else { // error
									cerr << "Error: Unexpected text in page range end, here: " << endl;
									cerr << "   " << argv[ii] << endl;
									cerr << "   Exiting." << endl;
									cerr << "   The acceptable keyword, here, is \"end\"." << endl;
									fail_b= true;
									break;
								}
							}

							jj+= keyword_len;
						}
					}

					////
					// pack this range into our m_page_seq; 

					if( page_num_beg== 0 && page_num_end== 0 ) { // ref the entire document
						page_num_beg= 1;
						page_num_end= m_input_pdf[range_pdf_index].m_num_pages;
					}
					else if( page_num_end== 0 ) { // a single page ref
						page_num_end= page_num_beg;
					}

					vector< PageRef > temp_page_seq;
					bool reverse_sequence_b= ( page_num_end< page_num_beg );
					if( reverse_sequence_b ) { // swap
						PageNumber temp= page_num_end;
						page_num_end= page_num_beg;
						page_num_beg= temp;
					}

					for( PageNumber kk= page_num_beg; kk<= page_num_end; ++kk ) {
						if( (!even_pages_b || !(kk % 2)) &&
								(!odd_pages_b || (kk % 2)) )
							{
								if( 0<= kk && kk<= m_input_pdf[range_pdf_index].m_num_pages ) {

									// look to see if this page of this document
									// has already been referenced; if it has,
									// create a new reader; associate this page
									// with a reader;
									//
									vector< pair< set<jint>, itext::PdfReader* > >::iterator it=
										m_input_pdf[range_pdf_index].m_readers.begin();
									for( ; it!= m_input_pdf[range_pdf_index].m_readers.end(); ++it ) {
										set<jint>::iterator jt= it->first.find( kk );
										if( jt== it->first.end() ) { // kk not assoc. w/ this reader
											it->first.insert( kk ); // create assoc.
											break;
										}
									}
									//
									if( it== m_input_pdf[range_pdf_index].m_readers.end() ) {
										// need to create a new reader for kk
										if( add_reader( &(m_input_pdf[range_pdf_index]) ) ) {
											m_input_pdf[range_pdf_index].m_readers.back().first.insert( kk );
										}
										else {
											cerr << "Internal Error: unable to add reader" << endl;
											fail_b= true;
											break;
										}
									}

									//
									temp_page_seq.push_back( PageRef(range_pdf_index, kk) );

								}
								else { // error; break later to get most feedback
									cerr << "Error: Page number: " << kk << endl;
									cerr << "   does not exist in file: " << m_input_pdf[range_pdf_index].m_filename << endl;
									fail_b= true;
								}
							}
					}
					if( fail_b )
						break;

					if( reverse_sequence_b ) {
						reverse( temp_page_seq.begin(), temp_page_seq.end() );
					}

					m_page_seq.insert( m_page_seq.end(), temp_page_seq.begin(), temp_page_seq.end() );

				} // end: handle page sequence
			else { // error
				cerr << "Error: expecting page ranges or \"output\" keyword." << endl;
				fail_b= true;
				break;
			}
    } // end: case:page_seq_e
    break;

    case output_filename_e: {
			// we have closed all possible input operations and arguments;
			// see if we should perform any default action based on the input state
			//
			if( m_operation== none_k ) {
				if( 1< m_input_pdf.size() ) {
					// no operation given for multiple input PDF, so combine them
					m_operation= cat_k;
				}
				else {
					m_operation= filter_k;
				}
			}
			
			// try opening input PDF readers (in case they aren't already)
			if( !open_input_pdf_readers() ) { // failure
				fail_b= true;
				break;
			}

			if( m_operation== cat_k &&
					m_page_seq.empty() )
				{ // combining pages, but no sequences given; merge all input PDFs in order
					for( InputPdfIndex ii= 0; ii< m_input_pdf.size(); ++ii ) {
						InputPdf& input_pdf= m_input_pdf[ii];

						for( PageNumber jj= 1; jj<= input_pdf.m_num_pages; ++jj ) {
							m_page_seq.push_back( PageRef( ii, jj ) );
							m_input_pdf[ii].m_readers.back().first.insert( jj ); // mark our claim
						}
					}
				}

			if( m_output_filename.empty() ) {
				m_output_filename= argv[ii];

				// simple-minded test to see if output matches an input filename
				for( vector< InputPdf >::const_iterator it= m_input_pdf.begin();
						 it!= m_input_pdf.end(); ++it )
					{
						if( it->m_filename== m_output_filename ) {
							cerr << "Error: The given output filename: " << m_output_filename << endl;
							cerr << "   matches an input filename.  Exiting." << endl;
							fail_b= true;
							break;
						}
					}
			}
			else { // error
				cerr << "Error: Multiple output filenames given: " << endl;
				cerr << "   " << m_output_filename << " and " << argv[ii] << endl;
				cerr << "Exiting." << endl;
				fail_b= true;
				break;
			}

			// advance state
			arg_state= output_args_e;
		}
		break;

		case output_args_e: {
			// output args are order-independent but must follow "output <fn>";
			// we are expecting any of these keywords:
			// owner_pw_k, user_pw_k, user_perms_k ...
			//
			switch( arg_keyword ) {

			case owner_pw_k:
				// change state
				arg_state= output_owner_pw_e;
				break;

			case user_pw_k:
				// change state
				arg_state= output_user_pw_e;
				break;

			case user_perms_k:
				// change state
				arg_state= output_user_perms_e;
				break;

				////
				// no arguments to these keywords, so the state remains unchanged
			case encrypt_40bit_k:
				m_output_encryption_strength= bits40_enc;
				break;
			case encrypt_128bit_k:
				m_output_encryption_strength= bits128_enc;
				break;
			case filt_uncompress_k:
				m_output_uncompress_b= true;
				break;
			case filt_compress_k:
				m_output_compress_b= true;
				break;
			case verbose_k:
				m_verbose_reporting_b= true;
				break;

			default: // error: unexpected matter
				cerr << "Error: Unexpected data in output section: " << endl;
				cerr << "      " << argv[ii] << endl;
				cerr << "Exiting." << endl;
				fail_b= true;
				break;
			}
		}
		break;

		case output_owner_pw_e: {
			if( m_output_owner_pw.empty() ) {
				if( m_output_user_pw!= argv[ii] ) {
					m_output_owner_pw= argv[ii];
				}
				else { // error: identical user and owner password
					// are interpreted by Acrobat (per the spec.) that
					// the doc has no owner password
					cerr << "Error: The user and owner passwords are the same." << endl;
					cerr << "   PDF Viewers interpret this to mean your PDF has" << endl;
					cerr << "   no owner password, so they must be different." << endl;
					cerr << "   Or, supply no owner password to pdftk if this is" << endl;
					cerr << "   what you desire." << endl;
					cerr << "Exiting." << endl;
					fail_b= true;
					break;
				}
			}
			else { // error: we already have an output owner pw
				cerr << "Error: Multiple output owner passwords given: " << endl;
				cerr << "   " << m_output_owner_pw << " and " << argv[ii] << endl;
				cerr << "Exiting." << endl;
				fail_b= true;
				break;
			}

			// revert state
			arg_state= output_args_e;
		}
		break;

		case output_user_pw_e: {
			if( m_output_user_pw.empty() ) {
				if( m_output_owner_pw!= argv[ii] ) {
					m_output_user_pw= argv[ii];
				}
				else { // error: identical user and owner password
					// are interpreted by Acrobat (per the spec.) that
					// the doc has no owner password
					cerr << "Error: The user and owner passwords are the same." << endl;
					cerr << "   PDF Viewers interpret this to mean your PDF has" << endl;
					cerr << "   no owner password, so they must be different." << endl;
					cerr << "   Or, supply no owner password to pdftk if this is" << endl;
					cerr << "   what you desire." << endl;
					cerr << "Exiting." << endl;
					fail_b= true;
					break;
				}
			}
			else { // error: we already have an output user pw
				cerr << "Error: Multiple output user passwords given: " << endl;
				cerr << "   " << m_output_user_pw << " and " << argv[ii] << endl;
				cerr << "Exiting." << endl;
				fail_b= true;
				break;
			}

			// revert state
			arg_state= output_args_e;
		}
		break;

		case output_user_perms_e: {
			using com::lowagie::text::pdf::PdfWriter;

			// we may be given any number of permission arguments,
			// so keep an eye out for other, state-altering keywords
			//
			switch( arg_keyword ) {

				// state-altering keywords
			case owner_pw_k:
				// change state
				arg_state= output_owner_pw_e;
				break;
			case user_pw_k:
				// change state
				arg_state= output_user_pw_e;
				break;
			case user_perms_k: // our current state
				break;

				// these don't change the state
			case encrypt_40bit_k:
				m_output_encryption_strength= bits40_enc;
				break;
			case encrypt_128bit_k:
				m_output_encryption_strength= bits128_enc;
				break;
			case filt_uncompress_k:
				m_output_uncompress_b= true;
				break;
			case filt_compress_k:
				m_output_compress_b= true;
				break;
			case verbose_k:
				m_verbose_reporting_b= true;
				break;

				// possible permissions
			case perm_printing_k:
				// if both perm_printing_k and perm_degraded_printing_k
				// are given, then perm_printing_k wins;
				m_output_user_perms|= 
					PdfWriter::AllowPrinting;
				break;
			case perm_modify_contents_k:
				// Acrobat 5 and 6 don't set both bits, even though
				// they both respect AllowModifyContents --> AllowAssembly;
				// so, no harm in this;
				m_output_user_perms|= 
					( PdfWriter::AllowModifyContents | PdfWriter::AllowAssembly );
				break;
			case perm_copy_contents_k:
				// Acrobat 5 _does_ allow the user to allow copying contents
				// yet hold back screen reader perms; this is counter-intuitive,
				// and Acrobat 6 does not allow Copy w/o SceenReaders;
				m_output_user_perms|= 
					( PdfWriter::AllowCopy | PdfWriter::AllowScreenReaders );
				break;
			case perm_modify_annotations_k:
				m_output_user_perms|= 
					( PdfWriter::AllowModifyAnnotations | PdfWriter::AllowFillIn );
				break;
			case perm_fillin_k:
				m_output_user_perms|= 
					PdfWriter::AllowFillIn;
				break;
			case perm_screen_readers_k:
				m_output_user_perms|= 
					PdfWriter::AllowScreenReaders;
				break;
			case perm_assembly_k:
				m_output_user_perms|= 
					PdfWriter::AllowAssembly;
				break;
			case perm_degraded_printing_k:
				m_output_user_perms|= 
					PdfWriter::AllowDegradedPrinting;
				break;
			case perm_all_k:
				m_output_user_perms= 
					( PdfWriter::AllowPrinting | // top quality printing
						PdfWriter::AllowModifyContents |
						PdfWriter::AllowCopy |
						PdfWriter::AllowModifyAnnotations |
						PdfWriter::AllowFillIn |
						PdfWriter::AllowScreenReaders |
						PdfWriter::AllowAssembly );
				break;

			default: // error: unexpected matter
				cerr << "Error: Unexpected data in output section: " << endl;
				cerr << "      " << argv[ii] << endl;
				cerr << "Exiting." << endl;
				fail_b= true;
				break;
			}
		}
		break;

    default: { // error
			cerr << "Internal Error: Unexpected arg_state.  Exiting." << endl;
			fail_b= true;
			break;
		}
		break;

    } // end: switch(arg_state)

  } // end: iterate over command-line arguments

	if( fail_b ) {
		cerr << "Errors encountered.  No output created." << endl;
		m_valid_b= false;

		g_dont_collect_p->clear();
		m_input_pdf.erase( m_input_pdf.begin(), m_input_pdf.end() );

		// preserve other data members for diagnostic dump
	}
	else {
		m_valid_b= true;

		if(!m_input_pdf_readers_opened_b ) {
			open_input_pdf_readers();
		}
	}
}

TK_Session::~TK_Session()
{
	g_dont_collect_p->clear();
}

int
GetPageNumber( itext::PdfDictionary* dict_p,
							 itext::PdfReader* reader_p )
// take a PdfPage dictionary and return its page location in the document;
// recurse our way up the pages tree, counting pages as we go;
// dict_p may be a page or a page tree object;
// return value is zero-based;
{
	if( dict_p && dict_p->contains( itext::PdfName::PARENT ) ) {
		jint sum_pages= 0;

		itext::PdfDictionary* parent_p= (itext::PdfDictionary*)
			reader_p->getPdfObject( dict_p->get( itext::PdfName::PARENT ) );
		if( parent_p && parent_p->isDictionary() ) {
			// a parent is a page tree object and will have Kids

			itext::PdfArray* parent_kids_p= (itext::PdfArray*)
				reader_p->getPdfObject( parent_p->get( itext::PdfName::KIDS ) );
			if( parent_kids_p && parent_kids_p->isArray() ) {
				// Kids may be Pages or Page Tree Nodes

				// iterate over *dict_p's parent's kids until we run into *dict_p
				java::ArrayList* kids_p= parent_kids_p->getArrayList();
				if( kids_p ) {
					for( jint kids_ii= 0; kids_ii< kids_p->size(); ++kids_ii ) {

						itext::PdfDictionary* kid_p= (itext::PdfDictionary*)
							reader_p->getPdfObject( (itext::PdfDictionary*)(kids_p->get(kids_ii)) );
						if( kid_p && kid_p->isDictionary() ) {

							if( kid_p== dict_p ) {
								// counting kids is complete; recurse

								return sum_pages+ GetPageNumber( parent_p, reader_p ); // <--- return
							}
							else {
								// is kid a page, or is kid a page tree object? add count to sum;
								// PdfDictionary::isPage() and PdfDictionary::isPages()
								// are not reliable, here

								itext::PdfName* kid_type_p= (itext::PdfName*)
									reader_p->getPdfObject( kid_p->get( itext::PdfName::TYPE ) );
								if( kid_type_p && kid_type_p->isName() ) {

									if( kid_type_p->compareTo( itext::PdfName::PAGE )== 0 ) {
										// *kid_p is a Page

										sum_pages+= 1;
									}
									else if( kid_type_p->compareTo( itext::PdfName::PAGES )== 0 ) {
										// *kid_p is a Page Tree Node

										itext::PdfNumber* count_p= (itext::PdfNumber*)
											reader_p->getPdfObject( kid_p->get( itext::PdfName::COUNT ) );
										if( count_p && count_p->isNumber() ) {

											sum_pages+= count_p->intValue();

										}
										else { // error
											cerr << "Internal Error: invalid count in GetPageNumber" << endl;
										}
									}
									else { // error
										cerr << "Internal Error: unexpected kid type in GetPageNumber" << endl;
									}
								}
								else { // error
									cerr << "Internal Error: invalid kid_type_p in GetPageNumber" << endl;
								}
							}
						}
						else { // error
							cerr << "Internal Error: invalid kid_p in GetPageNumber" << endl;
						}
					} // done iterating over kids

				}
				else { // error
					cerr << "Internal Error: invalid kids_p in GetPageNumber" << endl;
				}
			}
			else { // error
				cerr << "Internal Error: invalid kids array GetPageNumber" << endl;
			}
		}
		else { // error
			cerr << "Internal Error: invalid parent in GetPageNumber" << endl;
		}
	}
	else {
		// *dict_p has no parent; end recursion
		return 0;
	}

	// error: should have recursed
	cerr << "Internal Error: recursion case skipped in GetPageNumber" << endl;

	return 0;
}

static void 
OutputString( ostream& ofs,
							java::lang::String* jss_p )
{
	if( jss_p ) {
		for( jint ii= 0; ii< jss_p->length(); ++ii ) {
			jchar wc= jss_p->charAt(ii);
			if( 0x20<= wc && wc<= 0x7e ) {
				switch( wc ) {
				case '<' :
					ofs << "&lt;";
					break;
				case '>':
					ofs << "&gt;";
					break;
				case '&':
					ofs << "&amp;";
					break;
				case '"':
					ofs << "&quot;";
					break;
				default:
					ofs << (char)wc;
					break;
				}
			}
			else { // HTML numerical entity
				ofs << "&#" << (int)wc << ";";
			}
		}
	}
}

static void
OutputPdfString( ostream& ofs,
								 itext::PdfString* pdfss_p )
{
	if( pdfss_p && pdfss_p->isString() ) {
		java::lang::String* jss_p= pdfss_p->toUnicodeString();
		OutputString( ofs, jss_p );
	}
}

static void
OutputID( ostream& ofs,
					java::lang::String* id_p )
{
	ofs << hex;
	for( jint ii= 0; ii< id_p->length(); ++ii ) {
		unsigned int jc= (unsigned int)id_p->charAt(ii);
		ofs << jc;
	}
	ofs << dec;
}

static void
ReportOutlines( ostream& ofs, 
								itext::PdfDictionary* outline_p,
								int level,
								itext::PdfReader* reader_p )
{
	// the title; HTML-compatible
	ofs << "BookmarkTitle: ";
	itext::PdfString* title_p= (itext::PdfString*)
		reader_p->getPdfObject( outline_p->get( itext::PdfName::TITLE ) );
	if( title_p && title_p->isString() ) {

		OutputPdfString( ofs, title_p );
		
		ofs << endl;
	}
	else { // error
		ofs << "[ERROR: TITLE NOT FOUND]" << endl;
	}

	// the level; 1-based to jive with HTML heading level concept
	ofs << "BookmarkLevel: " << level+ 1 << endl;

	// page number, 1-based; 
	// a zero value indicates no page destination or an error
	ofs << "BookmarkPageNumber: ";
	{
		bool fail_b= false;

		// the destination object may take be in a couple different places
		// and may take a couple, different forms

		itext::PdfObject* destination_p= 0; {
			if( outline_p->contains( itext::PdfName::DEST ) ) {
				destination_p=
					reader_p->getPdfObject( outline_p->get( itext::PdfName::DEST ) );
			}
			else if( outline_p->contains( itext::PdfName::A ) ) {

				itext::PdfDictionary* action_p= (itext::PdfDictionary*)
					reader_p->getPdfObject( outline_p->get( itext::PdfName::A ) );
				if( action_p && action_p->isDictionary() ) {

					// TODO: confirm action subtype of GoTo
					itext::PdfName* s_p= (itext::PdfName*)
						reader_p->getPdfObject( action_p->get( itext::PdfName::S ) );
					if( s_p && s_p->isName() ) {

						if( s_p->compareTo( itext::PdfName::GOTO )== 0 ) {
							destination_p=
								reader_p->getPdfObject( action_p->get( itext::PdfName::D ) );
						}
						else { // immediate action is not a link in this document;
							// not an error

							fail_b= true;
						}
					}
					else { // error
						fail_b= true;
					}
				}
				else { // error
					fail_b= true;
				}
			}
			else { // unexpected
				fail_b= true;
			}
		}

		// destination is an array
		if( destination_p && destination_p->isArray() ) {

			java::ArrayList* array_list_p= ((itext::PdfArray*)destination_p)->getArrayList();
			if( array_list_p && !array_list_p->isEmpty() ) {

				itext::PdfDictionary* page_p= (itext::PdfDictionary*)
					reader_p->getPdfObject( (itext::PdfObject*)(array_list_p->get(0)) );

				if( page_p && page_p->isDictionary() ) {
					ofs << GetPageNumber(page_p, reader_p)+ 1 << endl;
				}
				else { // error
					fail_b= true;
				}
			}
			else { // error
				fail_b= true;
			}
		} // TODO: named destinations handling
		else { // error
			fail_b= true;
		}

		if( fail_b ) { // output our 'null page reference' code
			ofs << 0 << endl;
		}
	}

	// recurse into any children
	if( outline_p->contains( itext::PdfName::FIRST ) ) {

		itext::PdfDictionary* child_p= (itext::PdfDictionary*)
			reader_p->getPdfObject( outline_p->get( itext::PdfName::FIRST ) );
		if( child_p && child_p->isDictionary() ) {

			ReportOutlines( ofs, child_p, level+ 1, reader_p );
		}
	}

	// recurse into next sibling
	if( outline_p->contains( itext::PdfName::NEXT ) ) {

		itext::PdfDictionary* sibling_p= (itext::PdfDictionary*)
			reader_p->getPdfObject( outline_p->get( itext::PdfName::NEXT ) );
		if( sibling_p && sibling_p->isDictionary() ) {

			ReportOutlines( ofs, sibling_p, level, reader_p );
		}
	}
}

static void
ReportInfo( ostream& ofs,
						itext::PdfDictionary* info_p,
						itext::PdfReader* reader_p )
{
	if( info_p && info_p->isDictionary() ) {
		java::Set* keys_p= info_p->getKeys();

		// iterate over Info keys
		for( java::Iterator* it= keys_p->iterator(); it->hasNext(); ) {

			itext::PdfName* key_p= (itext::PdfName*)it->next();
			int key_len= JvGetArrayLength( key_p->getBytes() )- 1; // minus one for init. slash

			itext::PdfObject* value_p= (itext::PdfObject*)info_p->get( key_p );

			// don't output empty keys or values
			if( 0< key_len &&
					value_p->isString() && 
					0< ((itext::PdfString*)value_p)->toString()->length() ) 
				{ // ouput
					const int buff_size= 128;
					char buff[buff_size];
					memset( buff, 0, buff_size );

					// convert the PdfName into a c-string; omit initial slash
					strncpy( buff, 
									 (char*)elements( key_p->getBytes() )+ 1,
									 ( key_len< buff_size- 1 ) ? key_len : buff_size- 1 );

					ofs << "InfoKey: " << buff << endl;

					ofs << "InfoValue: ";
					OutputPdfString( ofs, (itext::PdfString*)value_p );
					ofs << endl;
				}
		}

	}
	else { // error
	}
}

static void
ReportPageLabels( ostream& ofs,
									itext::PdfDictionary* numtree_node_p,
									itext::PdfReader* reader_p )
	// if *numtree_node_p has Nums, report them;
	// else if *numtree_node_p has Kids, recurse
	// output 1-based page numbers; that's what we do for bookmarks
{
	itext::PdfArray* nums_p= (itext::PdfArray*)
		reader_p->getPdfObject( numtree_node_p->get( itext::PdfName::NUMS ) );
	if( nums_p && nums_p->isArray() ) {
		// report page numbers

		java::ArrayList* labels_p= nums_p->getArrayList();
		if( labels_p ) {
			for( jint labels_ii= 0; labels_ii< labels_p->size(); labels_ii+=2 ) {
				
				// label index
				itext::PdfNumber* index_p= (itext::PdfNumber*)
					reader_p->getPdfObject( (itext::PdfNumber*)(labels_p->get(labels_ii)) );

				// label dictionary
				itext::PdfDictionary* label_p= (itext::PdfDictionary*)
					reader_p->getPdfObject( (itext::PdfDictionary*)(labels_p->get(labels_ii+ 1)) );

				if( index_p && index_p->isNumber() &&
						label_p && label_p->isDictionary() )
					{
						// PageLabelNewIndex
						ofs << "PageLabelNewIndex: " << (long)(index_p->intValue())+ 1 << endl;
						
						{ // PageLabelStart
							ofs << "PageLabelStart: "; 
							itext::PdfNumber* start_p= (itext::PdfNumber*)
								reader_p->getPdfObject( label_p->get( itext::PdfName::ST ) );
							if( start_p && start_p->isNumber() ) {
								ofs << (long)(start_p->intValue()) << endl;
							}
							else {
								ofs << "1" << endl; // the default
							}
						}

						{ // PageLabelPrefix
							itext::PdfString* prefix_p= (itext::PdfString*)
								reader_p->getPdfObject( label_p->get( itext::PdfName::P ) );
							if( prefix_p && prefix_p->isString() ) {
								ofs << "PageLabelPrefix: ";
								OutputPdfString( ofs, prefix_p );
								ofs << endl;
							}
						}

						{ // PageLabelNumStyle
							itext::PdfName* r_p= new itext::PdfName(JvNewStringLatin1("r"));
							itext::PdfName* a_p= new itext::PdfName(JvNewStringLatin1("a"));

							itext::PdfName* style_p= (itext::PdfName*)
								reader_p->getPdfObject( label_p->get( itext::PdfName::S ) );
							ofs << "PageLabelNumStyle: ";
							if( style_p && style_p->isName() ) {
								if( style_p->compareTo( itext::PdfName::D )== 0 ) {
									ofs << "DecimalArabicNumerals" << endl;
								}
								else if( style_p->compareTo( itext::PdfName::R )== 0 ) {
									ofs << "UppercaseRomanNumerals" << endl;
								}
								else if( style_p->compareTo( r_p )== 0 ) {
									ofs << "LowercaseRomanNumerals" << endl;
								}
								else if( style_p->compareTo( itext::PdfName::A )== 0 ) {
									ofs << "UppercaseLetters" << endl;
								}
								else if( style_p->compareTo( a_p )== 0 ) {
									ofs << "LowercaseLetters" << endl;
								}
								else { // error
									ofs << "[ERROR]" << endl;
								}
							}
							else { // default
								ofs << "NoNumber" << endl;
							}
						}

					}
				else { // error
					ofs << "[ERROR: INVALID label_p IN ReportPageLabelNode]" << endl;
				}
			}
		}
		else { // error
			ofs << "[ERROR: INVALID labels_p IN ReportPageLabelNode]" << endl;
		}
	}
	else { // try recursing
		itext::PdfArray* kids_p= (itext::PdfArray*)
			reader_p->getPdfObject( numtree_node_p->get( itext::PdfName::KIDS ) );
		if( kids_p && kids_p->isArray() ) {

			java::ArrayList* kids_ar_p= kids_p->getArrayList();
			if( kids_ar_p ) {
				for( jint kids_ii= 0; kids_ii< kids_ar_p->size(); ++kids_ii ) {

					itext::PdfDictionary* kid_p= (itext::PdfDictionary*)
						reader_p->getPdfObject( (itext::PdfDictionary*)kids_ar_p->get(kids_ii) );
					if( kid_p && kid_p->isDictionary() ) {

						// recurse
						ReportPageLabels( ofs,
															kid_p,
															reader_p );
					}
					else { // error
						ofs << "[ERROR: INVALID kid_p]" << endl;
					}
				}
			}
			else { // error
				ofs << "[ERROR: INVALID kids_ar_p]" << endl;
			}
		}
		else { // error; a number tree must have one or the other
			ofs << "[ERROR: INVALID PAGE LABEL NUMBER TREE]" << endl;
		}
	}
}

static void
ReportOnPdf( ostream& ofs,
						 itext::PdfReader* reader_p )
{
	{ // trailer data
		itext::PdfDictionary* trailer_p= reader_p->getTrailer();
		if( trailer_p && trailer_p->isDictionary() ) {

			{ // metadata
				itext::PdfDictionary* info_p= (itext::PdfDictionary*)
					reader_p->getPdfObject( trailer_p->get( itext::PdfName::INFO ) );
				if( info_p && info_p->isDictionary() ) {
						
					ReportInfo( ofs, info_p, reader_p );
				}
				else { // warning
					cerr << "Warning: no info dictionary found" << endl;
				}
			}

			{ // pdf ID; optional
				itext::PdfArray* id_p= (itext::PdfArray*)
					reader_p->getPdfObject( trailer_p->get( itext::PdfName::ID ) );
				if( id_p && id_p->isArray() ) {

					java::ArrayList* id_al_p= id_p->getArrayList();
					if( id_al_p ) {

						for( jint ii= 0; ii< id_al_p->size(); ++ii ) {
							ofs << "PdfID" << (int)ii << ": ";

							itext::PdfString* id_ss_p= (itext::PdfString*)
								reader_p->getPdfObject( (itext::PdfObject*)id_al_p->get(ii) );
							if( id_ss_p && id_ss_p->isString() ) {
									
								OutputID( ofs, id_ss_p->toString() );
							}
							else { // error
								cerr << "Internal Error: invalid pdf id array string" << endl;
							}

							ofs << endl;
						}
					}
					else { // error
						cerr << "Internal Error: invalid ID ArrayList" << endl;
					}
				}
			}

		}
		else { // error
			cerr << "InternalError: invalid trailer" << endl;
		}
	}

	{ // number of pages and outlines
		itext::PdfDictionary* catalog_p= reader_p->catalog;
		if( catalog_p && catalog_p->isDictionary() ) {

			// number of pages
			itext::PdfDictionary* pages_p= (itext::PdfDictionary*)
				reader_p->getPdfObject( catalog_p->get( itext::PdfName::PAGES ) );
			if( pages_p && pages_p->isDictionary() ) {

				itext::PdfNumber* count_p= (itext::PdfNumber*)
					reader_p->getPdfObject( pages_p->get( itext::PdfName::COUNT ) );
				if( count_p && count_p->isNumber() ) {

					ofs << "NumberOfPages: " << (unsigned int)count_p->intValue() << endl;
				}
				else { // error
					cerr << "Internal Error: invalid count_p in ReportOnPdf()" << endl;
				}
			}
			else { // error
				cerr << "Internal Error: invalid pages_p in ReportOnPdf()" << endl;
			}

			// outlines; optional
			itext::PdfDictionary* outlines_p= (itext::PdfDictionary*)
				reader_p->getPdfObject( catalog_p->get( itext::PdfName::OUTLINES ) );
			if( outlines_p && outlines_p->isDictionary() ) {

				itext::PdfDictionary* top_outline_p= (itext::PdfDictionary*)
					reader_p->getPdfObject( outlines_p->get( itext::PdfName::FIRST ) );
				if( top_outline_p && top_outline_p->isDictionary() ) {

					ReportOutlines( ofs, top_outline_p, 0, reader_p );
				}
				else { // error
					cerr << "Internal Error: invalid top_outline_p in ReportOnPdf()" << endl;
				}
			}

		}
		else { // error
			cerr << "InternalError:" << endl;
		}
	}

	{ // page labels (a/k/a logical page numbers)
		itext::PdfDictionary* catalog_p= reader_p->catalog;
		if( catalog_p && catalog_p->isDictionary() ) {

			itext::PdfDictionary* pagelabels_p= (itext::PdfDictionary*)
				reader_p->getPdfObject( catalog_p->get( itext::PdfName::PAGELABELS ) );
			if( pagelabels_p && pagelabels_p->isDictionary() ) {

				ReportPageLabels( ofs,
													pagelabels_p,
													reader_p );
			}
		}
		else { // error
			cerr << "InternalError:" << endl;
		}
	}

} // end: ReportOnPdf

void
TK_Session::create_output()
{
	if( is_valid() ) {
		if( m_verbose_reporting_b ) {
			cout << endl << "Creating Output ..." << endl;
		}

		itext::Document* output_doc_p= 0;
		itext::PdfCopy* writer_p= 0;
		java::FileOutputStream* ofs_p= 0;

		java::String* jv_creator_p= 
			JvNewStringLatin1( "pdftk 0.94 - www.pdftk.com" );
		java::String* jv_output_filename_p= 
			JvNewStringLatin1( m_output_filename.c_str() );

		jbyteArray output_owner_pw_p= JvNewByteArray( m_output_owner_pw.size() ); {
			jbyte* pw_p= elements(output_owner_pw_p);
			memcpy( pw_p, m_output_owner_pw.c_str(), m_output_owner_pw.size() ); 
		}
		jbyteArray output_user_pw_p= JvNewByteArray( m_output_user_pw.size() ); {
			jbyte* pw_p= elements(output_user_pw_p);
			memcpy( pw_p, m_output_user_pw.c_str(), m_output_user_pw.size() ); 
		}

		try {
			switch( m_operation ) {

			case cat_k : { // Catenation
				output_doc_p= new itext::Document();

				ofs_p= new java::FileOutputStream( jv_output_filename_p );
				writer_p= new itext::PdfCopy( output_doc_p, ofs_p );

				output_doc_p->addCreator( jv_creator_p );

				// output encryption
				if( m_output_encryption_strength!= none_enc ||
						!m_output_owner_pw.empty() || 
						!m_output_user_pw.empty() )
					{
						// if no stregth is given, default to 128 bit,
						// (which is incompatible w/ Acrobat 4)
						bool bit128_b=
							( m_output_encryption_strength!= bits40_enc );

						writer_p->setEncryption( output_user_pw_p,
																		 output_owner_pw_p,
																		 m_output_user_perms,
																		 bit128_b );
					}

				output_doc_p->open();

				for( vector< PageRef >::const_iterator it= m_page_seq.begin();
						 it!= m_page_seq.end(); ++it )
					{
						// get the reader associated with this page ref.
						//map< string, InputPdfIndex >::iterator jt= 
						//	m_input_pdf_index.find( it->m_handle );
						if( it->m_input_pdf_index< m_input_pdf.size() ) {
							InputPdf& input_pdf= m_input_pdf[ it->m_input_pdf_index ];

							if( m_verbose_reporting_b ) {
								cout << "   Adding page " << it->m_page_num;
								cout << " from " << input_pdf.m_filename << endl;
							}

							// take the first, associated reader and then disassociate
							itext::PdfReader* input_reader_p= 0;
							vector< pair< set<jint>, itext::PdfReader* > >::iterator mt=
								input_pdf.m_readers.begin();
							for( ; mt!= input_pdf.m_readers.end(); ++mt ) {
								set<jint>::iterator nt= mt->first.find( it->m_page_num );
								if( nt!= mt->first.end() ) { // assoc. found
									input_reader_p= mt->second;
									mt->first.erase( nt ); // remove this assoc.
									break;
								}
							}

							if( input_reader_p ) {
								itext::PdfImportedPage* page_p= 
									writer_p->getImportedPage( input_reader_p, it->m_page_num );

								writer_p->addPage( page_p );
							}
							else { // error
								cerr << "Internal Error: no reader found for page: ";
								cerr << it->m_page_num << " in file: " << input_pdf.m_filename << endl;
								break;
							}
						}
						else { // error
							cerr << "Internal Error: Unable to find handle in m_input_pdf." << endl;
							break;
						}
					}

				output_doc_p->close();
				writer_p->close();
			}
			break;
			
			case burst_k : { // Burst input into pages

				// we should have been given only a single, input file
				if( 1< m_input_pdf.size() ) { // error
					cerr << "Error: Only one input PDF file may be given for \"burst\" op." << endl;
					cerr << "   No output created." << endl;
					break;
				}

				// grab the first reader, since there's only one
				itext::PdfReader* input_reader_p= 
					m_input_pdf.begin()->m_readers.front().second;
				jint input_num_pages= 
					m_input_pdf.begin()->m_num_pages;

				for( jint ii= 0; ii< input_num_pages; ++ii ) {

					// the filename
					char buff[32]= "";
					sprintf( buff, "pg_%04d.pdf", ii+ 1 );
					jv_output_filename_p= JvNewStringLatin1( buff );

					output_doc_p= new itext::Document();
					ofs_p= new java::FileOutputStream( jv_output_filename_p );
					writer_p= new itext::PdfCopy( output_doc_p, ofs_p );

					output_doc_p->addCreator( jv_creator_p );

					// output encryption
					if( m_output_encryption_strength!= none_enc ||
							!m_output_owner_pw.empty() || 
							!m_output_user_pw.empty() )
						{
							// if no stregth is given, default to 128 bit,
							// (which is incompatible w/ Acrobat 4)
							bool bit128_b=
								( m_output_encryption_strength!= bits40_enc );

							writer_p->setEncryption( output_user_pw_p,
																			 output_owner_pw_p,
																			 m_output_user_perms,
																			 bit128_b );
						}

					output_doc_p->open();
						
					itext::PdfImportedPage* page_p= 
						writer_p->getImportedPage( input_reader_p, ii+ 1 );
						
					writer_p->addPage( page_p );

					output_doc_p->close();
					writer_p->close();
				}

				////
				// dump document data

				ofstream ofs( "doc_data.txt" );
				if( ofs ) {
					ReportOnPdf( ofs, input_reader_p );
				}
				else { // error
					cerr << "Error: unable to open file for output: doc_data.txt" << endl;
				}

			}
			break;

			case filter_k: {

				// we should have been given only a single, input file
				if( 1< m_input_pdf.size() ) { // error
					cerr << "Error: Only one input PDF file may be given whem omitting the" << endl;
					cerr << "   operator.Maybe you meant to use the \"cat\" operator?" << endl;
					cerr << "   No output created." << endl;
					break;
				}

				ofs_p= new java::FileOutputStream( jv_output_filename_p );
				itext::PdfReader* input_reader_p= 
					m_input_pdf.begin()->m_readers.front().second;

				if( m_output_encryption_strength!= none_enc ||
						!m_output_owner_pw.empty() ||
						!m_output_user_pw.empty() )
					{ // encrypt output

						// if no stregth is given, default to 128 bit,
						// (which is incompatible w/ Acrobat 4)
						bool bit128_b=
							( m_output_encryption_strength!= bits40_enc );
						
						itext::PdfEncryptor::encrypt( input_reader_p,
																					ofs_p,
																					output_user_pw_p,
																					output_owner_pw_p,
																					m_output_user_perms,
																					bit128_b );
					}
				else if( m_output_compress_b ||
								 m_output_uncompress_b )
					{ // modify output compression

						itext::PdfStamper* stamper_p= 
							new itext::PdfStamper( input_reader_p, ofs_p );

						stamper_p->getWriter()->filterStreams= m_output_uncompress_b;
						stamper_p->getWriter()->compressStreams= m_output_compress_b;

						stamper_p->close();
					}
				else { // pass-through filter

					itext::PdfStamper* stamper_p= 
						new itext::PdfStamper( input_reader_p, ofs_p );

					stamper_p->close();
				}

			}
			break;

			case dump_data_k: { // report on input document

				// we should have been given only a single, input file
				if( 1< m_input_pdf.size() ) { // error
					cerr << "Error: Only one input PDF file may be given for \"dump_data\" op." << endl;
					cerr << "   No output created." << endl;
					break;
				}

				itext::PdfReader* input_reader_p= 
					m_input_pdf.begin()->m_readers.front().second;

				if( m_output_filename.empty() ) {
					ReportOnPdf( cout, input_reader_p );
				}
				else {
					ofstream ofs( m_output_filename.c_str() );
					if( ofs ) {
						ReportOnPdf( ofs, input_reader_p );
					}
					else { // error
						cerr << "Error: unable to open file for output: " << m_output_filename << endl;
					}
				}
			}
			break;

			}
		}
		catch( java::lang::Throwable* t_p )
			{
				cerr << "Unhandled Java Exception:" << endl;
				t_p->printStackTrace();
			}
	}
}

int main(int argc, char** argv)
{
	bool version_b= false;
	bool describe_b= ( argc== 1 );
	int ret_val= 0; // default: no error

	for( int ii= 1; ii< argc; ++ii ) {
		version_b=
			(version_b || 
			 strcmp( argv[ii], "--version" )== 0 );
		describe_b= 
			(describe_b || 
			 strcmp( argv[ii], "--help" )== 0 || 
			 strcmp( argv[ii], "-h" )== 0 );
	}

	if( version_b ) {
		describe_header();
	}
	else if( describe_b ) {
		describe_usage();
	}
	else {
		try {
			JvCreateJavaVM(NULL);
			JvAttachCurrentThread(NULL, NULL);

			JvInitClass(&java::System::class$);
			JvInitClass(&java::util::ArrayList::class$);
			JvInitClass(&java::util::Iterator::class$);

			JvInitClass(&itext::PdfObject::class$);
			JvInitClass(&itext::PdfName::class$);
			JvInitClass(&itext::PdfDictionary::class$);
			JvInitClass(&itext::PdfOutline::class$);

			TK_Session tk_session( argc, argv );

			tk_session.dump_session_data();

			if( tk_session.is_valid() ) {
				tk_session.create_output();
			}
			else { // error
				cerr << "Done.  Input errors, so no output created." << endl;
				ret_val= 1;
			}

			JvDetachCurrentThread();
		}
		catch( java::lang::Throwable* t_p )
			{
				cerr << "Unhandled Java Exception:" << endl;
				t_p->printStackTrace();
				ret_val= 2;
			}
	}

	return ret_val;
}

static void
describe_header() {
	cout << endl;
	cout << "pdftk " << PDFTK_VER << " a Handy Tool for Manipulating PDF Documents" << endl;
	cout << "Copyright (C) 2003-04, Sid Steward - Please Visit: www.pdftk.com" << endl;
	cout << "This is free software; see the source code for copying conditions. There is" << endl;
	cout << "NO warranty, not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." << endl;
	cout << endl;
}

static void
describe_usage() {
	describe_header();

	cout << 
"SYNOPSIS\n\
       pdftk <input PDF files>\n\
	     [input_pw <input PDF owner passwords>]\n\
	     [<operation> <operation arguments>]\n\
	     [output <output filename>]\n\
	     [encrypt_40bit | encrypt_128bit] [allow <permissions>]\n\
	     [owner_pw <owner password>] [user_pw <user password>]\n\
	     [compress | uncompress]\n\
	     [verbose]\n\
\n\
\n\
       --help, -h\n\
	      Show summary of options.\n\
\n\
       <input PDF files>\n\
	      A list of the input PDF files. If you plan to combine these PDFs\n\
	      (without using handles) then list files in the  order  you  want\n\
	      them  combined.	Input  files  can  be associated with handles,\n\
	      where a handle is a single, upper-case letter:\n\
\n\
	      <input PDF handle>=<input PDF filename>\n\
\n\
	      Handles are often omitted.  They are useful when specifying  PDF\n\
	      passwords or page ranges, later.\n\
\n\
	      For example: A=input1.pdf B=input2.pdf\n\
\n\
       [input_pw <input PDF owner passwords>]\n\
	      Input  PDF  owner  passwords,  if necessary, are associated with\n\
	      files by using their handles:\n\
\n\
	      <input PDF handle>=<input PDF file owner password>\n\
\n\
	      If handles are not given, then  passwords  are  associated  with\n\
	      input files by order.\n\
\n\
	      Most  pdftk features require that encrypted input PDF are accom-\n\
	      panied by the ~owner~ password. If the input PDF	has  no  owner\n\
	      password, then the user password must be given, instead.	If the\n\
	      input PDF has no passwords, then no password should be given.\n\
\n\
       [<operation> <operation arguments>]\n\
	      If this optional argument is omitted, then pdftk runs  in  'fil-\n\
	      ter'  mode.   Filter mode takes only one PDF input and creates a\n\
	      new PDF after applying all of the output arguments, like encryp-\n\
	      tion and compression.\n\
\n\
	      Available  operations  are:  cat, burst, and dump_data. Only cat\n\
	      takes additional arguments in the form of page ranges, described\n\
	      below.\n\
\n\
	      cat    Catenates	pages  from  input  PDFs  to create a new PDF.\n\
		     Page order in the new PDF is specified by	the  order  of\n\
		     the  given  page  ranges.	Page ranges are described like\n\
		     this:\n\
\n\
		     <input PDF handle>[<begin page  number>[-<end  page  num-\n\
		     ber>[<qualifier>]]]\n\
\n\
		     Where  the  handle identifies one of the input PDF files,\n\
		     and the beginning and ending page numbers	are  one-based\n\
		     references  to  pages  in the PDF file, and the qualifier\n\
		     can be even or odd.\n\
\n\
		     If the handle is omitted, then the pages are  taken  from\n\
		     the first input PDF.\n\
\n\
		     If  no  arguments	are passed to cat, then pdftk combines\n\
		     all input PDFs in the order they were given to create the\n\
		     output.\n\
\n\
		     NOTES:\n\
		     * The end page number may be less than begin page number.\n\
		     * The keyword end may be used to reference the final page\n\
		     of a document instead of a page number.\n\
		     *	Reference  a  single  page by omitting the ending page\n\
		     number.\n\
		     *	The  handle  may be used alone to represent the entire\n\
		     PDF document, i.e., B1-end is the same as B.\n\
\n\
		     Page range examples:\n\
		     A1-21\n\
		     Bend-1odd\n\
		     A72\n\
		     A1-21 Beven A72\n\
\n\
	      burst  Splits  a	single,  input	PDF  document  into individual\n\
		     pages.  Do not provide an output section. Encryption  can\n\
		     be   applied   by	 appending   output   arguments,  like\n\
		     encrypt_128bit, e.g.:\n\
\n\
		     pdftk in.pdf burst owner_pw foopass encrypt_128bit\n\
\n\
	      dump_data\n\
		     Reads a  single,  input  PDF  file  and  reports  various\n\
		     statistics,  metadata,  and  bookmarks  (outlines) to the\n\
		     given output filename or to stdout.  Does	not  create  a\n\
		     new PDF.\n\
\n\
       [output <output filename>]\n\
	      The  output  PDF filename may not be set to the name of an input\n\
	      filename.\n\
\n\
       [encrypt_40bit | encrypt_128bit]\n\
	      If an output PDF user or owner password  is  given,  output  PDF\n\
	      encryption  strength defaults to 128 bits.  This can be overrid-\n\
	      den by specifying encrypt_40bit.\n\
\n\
       [allow <permissions>]\n\
	      Permissions are applied to the output PDF only if an  encryption\n\
	      strength is specified or an owner or user password is given.  If\n\
	      permissions are not specified, they  default  to	'none,'  which\n\
	      means all of the following features are disabled.\n\
\n\
	      The permissions section may include one or more of the following\n\
	      features:\n\
\n\
	      Printing\n\
		     Top Quality Printing\n\
\n\
	      DegradedPrinting\n\
		     Lower Quality Printing\n\
\n\
	      ModifyContents\n\
\n\
	      Assembly\n\
\n\
	      CopyContents\n\
\n\
	      ScreenReaders\n\
\n\
	      ModifyAnnotations\n\
\n\
	      FillIn\n\
\n\
	      AllFeatures\n\
		     Allows the user to perform all of the above.\n\
\n\
       [owner_pw <owner password>] [user_pw <user password>]\n\
	      If an encryption strength is given but  no  passwords  are  sup-\n\
	      plied,  then  the  owner	and user passwords remain empty, which\n\
	      means that the resulting PDF may	be  opened  and  its  security\n\
	      parameters altered by anybody.\n\
\n\
       [compress | uncompress]\n\
	      These  are  only useful when you want to edit PDF code in a text\n\
	      editor like vim or emacs.  Remove PDF page stream compression by\n\
	      applying	the  uncompress  filter.  Use  the  compress filter to\n\
	      restore compression.\n\
\n\
       [verbose]\n\
	      By default, pdftk runs quietly. Append verbose to the end and it\n\
	      will speak up.\n\
\n\
EXAMPLES\n\
       Decrypt a PDF\n\
	      pdftk secured.pdf input_pw foopass output unsecured.pdf\n\
\n\
       Encrypt	a  PDF using 128-bit strength (the default), withhold all per-\n\
       missions (the default)\n\
	      pdftk 1.pdf output 1.128.pdf owner_pw foopass\n\
\n\
       Same  as  above, except password 'baz' must also be used to open output\n\
       PDF\n\
	      pdftk 1.pdf output 1.128.pdf owner_pw foo user_pw baz\n\
\n\
       Same as above, except printing is allowed (once the PDF is open)\n\
	      pdftk  1.pdf  output  1.128.pdf  owner_pw  foo user_pw baz allow\n\
	      printing\n\
\n\
       Join in1.pdf and in2.pdf into a new PDF, out1.pdf\n\
	      pdftk in1.pdf in2.pdf cat output out1.pdf\n\
	      or (using handles):\n\
	      pdftk A=in1.pdf B=in2.pdf cat A B output out1.pdf\n\
	      or (using wildcards):\n\
	      pdftk *.pdf cat output combined.pdf\n\
\n\
       Remove 'page 13' from in1.pdf to create out1.pdf\n\
	      pdftk in.pdf cat 1-12 14-end output out1.pdf\n\
	      or:\n\
	      pdftk A=in1.pdf cat A1-12 A14-end output out1.pdf\n\
\n\
       Apply 40-bit  encryption  to  output,  revoking	all  permissions  (the\n\
       default). Set the owner PW to 'foopass'.\n\
	      pdftk  1.pdf  2.pdf  cat	output	3.pdf  encrypt_40bit  owner_pw\n\
	      foopass\n\
\n\
       Join  two files, one of which requires the password 'foopass'. The out-\n\
       put is not encrypted.\n\
	      pdftk A=secured.pdf 2.pdf input_pw A=foopass cat output 3.pdf\n\
\n\
       Uncompress PDF page streams for editing the PDF in a text editor (e.g.,\n\
       vim, emacs)\n\
	      pdftk doc.pdf output doc.unc.pdf uncompress\n\
\n\
       Repair a PDF's corrupted XREF table and stream lengths, if possible\n\
	      pdftk broken.pdf output fixed.pdf\n\
\n\
       Burst   a  single  PDF  document  into  pages  and  dump  its  data  to\n\
       doc_data.txt\n\
	      pdftk mydoc.pdf burst\n\
\n\
       Burst  a  single  PDF  document into encrypted pages. Allow low-quality\n\
       printing\n\
	      pdftk mydoc.pdf burst owner_pw foopass allow DegradedPrinting\n\
\n\
       Write a report on PDF document metadata and bookmarks to report.txt\n\
	      pdftk mydoc.pdf dump_data output report.txt\n";

}
