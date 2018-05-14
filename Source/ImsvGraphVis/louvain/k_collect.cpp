// File: k_collect.cpp
// -- conversion of a graph from ascii to binary, sample main file
//-----------------------------------------------------------------------------
// Community detection 
// Based on the article "Fast unfolding of community hierarchies in large networks"
// Copyright (C) 2008 V. Blondel, J.-L. Guillaume, R. Lambiotte, E. Lefebvre
//
// And based on the article
// Copyright (C) 2013 R. Campigotto, P. Conde Céspedes, J.-L. Guillaume
//
// This file is part of Louvain algorithm.
// 
// Louvain algorithm is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Louvain algorithm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with Louvain algorithm.  If not, see <http://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------------
// Author   : E. Lefebvre, adapted by J.-L. Guillaume and R. Campigotto
// Email    : jean-loup.guillaume@lip6.fr
// Location : Paris, France
// Time	    : July 2013
//-----------------------------------------------------------------------------
// see README.txt for more details


#include "graph.h"
#include "graph_binary.h"
#include "louvain.h"
#include <io.h>
#include <process.h>

#include "modularity.h"

using namespace std;


char *infile = NULL;
char *outfile = NULL;
char *outfile_w = NULL;
char *rel = NULL;
int type = UNWEIGHTED;
bool do_renumber = false;

int nb_pass = 0;
long double precision = 0.000001L;
int display_level = -2;

unsigned short id_qual = 0;

Quality *q;

bool verbose = false;


void
usage(char *prog_name, const char *more) {
  cerr << more;
  cerr << "usage: " << prog_name << " -i input_file -o outfile [-r outfile_relation] [-w outfile_weight] [-h]" << endl << endl;
  cerr << "read the graph and convert it to binary format" << endl;
  cerr << "-r file\tnodes are renumbered from 0 to nb_nodes-1 (the labelings connection is stored in a separate file)" << endl;
  cerr << "-w file\tread the graph as a weighted one and writes the weights in a separate file" << endl;
  cerr << "-h\tshow this usage message" << endl;
  exit(0);
}

void
parse_args(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      switch(argv[i][1]) {
      case 'i':
	if (i==argc-1)
	  usage(argv[0], "Infile missing\n");
	infile = argv[i+1];
	i++;
	break;
      case 'o':
	if (i==argc-1)
	  usage(argv[0], "Outfile missing\n");
        outfile = argv[i+1];
	i++;
	break;
      case 'w':
	type = WEIGHTED;
        outfile_w = argv[i+1];
	i++;
	break;
      case 'r':
	if (i==argc-1)
	  usage(argv[0], "Labelings connection outfile missing\n");
        rel = argv[i+1];
	i++;
	do_renumber=true;
	break;
      case 'e':
	precision = atof(argv[i+1]);
	i++;
	break;
      case 'v':
	verbose = true;
      case 'l':
	display_level = atoi(argv[i+1]);
	i++;
	break;
      default:
	usage(argv[0], "Unknown option\n");
      }
    } else {
      usage(argv[0], "More than one filename\n");
    }
  }
  if (infile==NULL)// || outfile==NULL)
    usage(argv[0], "In or outfile missing\n");
}

vector<pair<int, int> >
process_input(char *filename, int type) {
  ifstream finput;
  vector<pair<int, int> > flat_links;

  finput.open(filename,fstream::in);
  if (finput.is_open() != true) {
	cerr << "The file " << filename << " does not exist" << endl;
	exit(EXIT_FAILURE);
  }

  while (!finput.eof()) {
    unsigned int src, dest;
    long double weight = 1.0L;

    if (type==WEIGHTED) {
      finput >> src >> dest >> weight;
    } else {
      finput >> src >> dest;
    }

    if (finput) {
      flat_links.push_back(make_pair(src,dest));
    }
  }

  finput.close();

  return flat_links;
}

void
display_time(const char *str) {
  time_t rawtime;
  time ( &rawtime );
  cerr << str << ": " << ctime (&rawtime);
}

void
init_quality(GraphB *gb, unsigned short nbc) {
  
  if (nbc > 0)
    delete q;

  switch (id_qual) {
  case 0:
    q = new Modularity(*gb);
    break;
  default:
    q = new Modularity(*gb);
    break;
  }
}
    

int
main(int argc, char **argv) {
  parse_args(argc, argv);

  vector<pair<int, int> > flat_links = process_input(infile, type);

  Graph g(infile, type);

  g.clean(type);

  if (do_renumber)
    g.renumber(type, rel);

  stringstream data_stream;
  //g.display_binary(outfile, outfile_w, type);
  g.stream_binary(data_stream, type);

  srand(time(NULL) + _getpid());

  time_t time_begin, time_end;
  time(&time_begin);

  unsigned short nb_calls = 0;

  if (verbose)
    display_time("Begin");

  GraphB gb(data_stream, type);
  init_quality(&gb, nb_calls);
  nb_calls++;

  if (verbose)
    cerr << endl << "Computation of communities the " << q->name << " quality function" << endl << endl;

  Louvain c(-1, precision, q);
  
  //if (filename_part!=NULL)
  //  c.init_partition(filename_part);
  //
  bool improvement = true;

  long double quality = (c.qual)->quality();
  long double new_qual;

  int level = 0;
  int cumulative = 0;

  do {
    if (verbose) {
      cerr << "level " << level << ":\n";
      display_time("  start computation");
      cerr << "  network size: "
	   << (c.qual)->gb.nb_nodes << " nodes, "
	   << (c.qual)->gb.nb_links << " links, "
	   << (c.qual)->gb.total_weight << " total_weight" << endl;
    }

    improvement = c.one_level();
    cerr << "Improvement: " << std::boolalpha << improvement << endl;

    new_qual = (c.qual)->quality();

    if (++level==display_level)
      (c.qual)->gb.display();
    if (display_level==-1)
      c.display_partitionK(cumulative, improvement);

    cumulative += gb.nb_nodes;
    gb = c.partition2graph_binary();
    init_quality(&gb, nb_calls);
    nb_calls++;

    c = Louvain(-1, precision, q);

    if (verbose)
      cerr << "  quality increased from " << quality << " to " << new_qual << endl;

    quality = new_qual;

    if (verbose)
      display_time("  end computation");

    //if (filename_part!=NULL && level==1) // do at least one more computation if partition is provided
        //improvement=true;
  } while(improvement);

  time(&time_end);
  if (verbose) {
    display_time("End");
    cerr << "Total duration: " << (time_end-time_begin) << " src " << endl;
  }
  cerr << new_qual << endl;

  delete q;
}
