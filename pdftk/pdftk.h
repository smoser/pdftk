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

class TK_Session {
	
	bool m_valid_b;
	bool m_authorized_b;
	bool m_input_pdf_readers_opened_b; // have m_input_pdf readers been opened?
	bool m_verbose_reporting_b;
	bool m_ask_about_warnings_b;

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
	// pack input PDF in the order they're given on the command line
	vector< InputPdf > m_input_pdf;
	typedef vector< InputPdf >::size_type InputPdfIndex;

	// store input PDF handles here
	map< string, InputPdfIndex > m_input_pdf_index;

	bool add_reader( InputPdf* input_pdf_p );
	bool open_input_pdf_readers();

	vector< string > m_input_attach_file_filename;
	jint m_input_attach_file_pagenum;

	string m_update_info_filename;

  enum keyword {
    none_k= 0,

    // the operations
    cat_k, // combine pages from input PDFs into a single output
		burst_k, // split a single, input PDF into individual pages
		filter_k, // apply 'filters' to a single, input PDF based on output args
		dump_data_k, // no PDF output
		dump_data_fields_k,
		generate_fdf_k,
		unpack_files_k, // unpack files from input; no PDF output
		//
		first_operation_k= cat_k,
    final_operation_k= unpack_files_k,

		// these are treated the same as operations,
		// but they are processed using the filter operation
		fill_form_k, // read FDF file and fill PDF form fields
		attach_file_k, // attach files to output
		update_info_k,
		background_k, // promoted from output option to operation in pdftk 1.10

		// optional attach_file argument
		attach_file_to_page_k,

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

		// forms
		flatten_k,

		// pdftk options
		verbose_k,
		dont_ask_k,
		do_ask_k
  };
  static keyword is_keyword( char* ss, int* keyword_len_p );

  keyword m_operation;

  typedef unsigned long PageNumber;
  typedef enum { NORTH= 0, EAST= 90, SOUTH= 180, WEST= 270 } PageRotate; // DF rotation
  typedef bool PageRotateAbsolute; // DF absolute / relative rotation
  
  struct PageRef {
		InputPdfIndex m_input_pdf_index;
    PageNumber m_page_num; // 1-based
    PageRotate m_page_rot; // DF rotation
    PageRotateAbsolute m_page_abs; //DF absolute / relative rotation

		PageRef( InputPdfIndex input_pdf_index, PageNumber page_num, PageRotate page_rot, PageRotateAbsolute page_abs ) :
						m_input_pdf_index( input_pdf_index ), m_page_num( page_num ), m_page_rot( page_rot ), m_page_abs( page_abs ) {}
		
  };
  vector< PageRef > m_page_seq;

	string m_form_data_filename;
	string m_background_filename;
  string m_output_filename;
	string m_output_owner_pw;
	string m_output_user_pw;
	jint m_output_user_perms;
	bool m_output_uncompress_b;
	bool m_output_compress_b;
	bool m_output_flatten_b;

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

	void attach_files
	( itext::PdfReader* input_reader_p,
		itext::PdfWriter* writer_p );

	void unpack_files
	( itext::PdfReader* input_reader_p );

	void create_output();

private:
	enum ArgState {
    input_files_e,
		input_pw_e,

    page_seq_e,
		form_data_filename_e,
		
		attach_file_filename_e,
		attach_file_pagenum_e,

		update_info_filename_e,

		output_e, // state where we expect output_k, next
    output_filename_e,

		output_args_e, // output args are order-independent; switch here
		output_owner_pw_e,
		output_user_pw_e,
		output_user_perms_e,

		background_filename_e,

		done_e
	};

	// convenience function; return true iff handled
	bool handle_some_output_options( TK_Session::keyword kw, ArgState* arg_state_p );

};

void
prompt_for_filename( const string fn_name,
										 string& fn );