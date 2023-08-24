/*
   Based on papers:
   PAM: Parallel Augmented maps
   Yihan Sun, Daniel Ferizovic and Guy Blelloch
   PPoPP 2018
   Parallel Range, Segment and Rectangle Queries with Augmented Maps
   Yihan Sun and Guy Blelloch
   ALENEX 2019
*/

#include <../common/time_loop.h>
#include <cpam/get_time.h>
#include <pam/parse_command_line.h>

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

#include "range_tree.h"

#define LOG std::cout
#define ENDL std::endl << std::flush

using RQ = RangeQuery<data_type, data_type>;
using points = sequence<point_type>;

void
reset_timers()
{
   reserve_tm.reset();
   init_tm.reset();
   sort_tm.reset();
   build_tm.reset();
   total_tm.reset();
}

auto
run_all( sequence<point_type>& points, size_t iteration, data_type min_val,
         data_type max_val, size_t query_num )
{
   double build_time;
   double size_in_gib;
   double query_time;
   {
      string benchmark_name = "Query-All";
      std::cout << "Building tree" << std::endl;
      RQ r( points );
      std::cout << "Built tree" << std::endl;

      sequence<Query_data> queries =
          generate_queries( query_num, min_val, max_val );

      sequence<size_t> counts( query_num );
      timer t_query_total;
      t_query_total.start();
      parallel_for(
          0, query_num,
          [&]( size_t i )
          {
             sequence<pair<int, int>> out = r.query_range(
                 queries[i].x1, queries[i].y1, queries[i].x2, queries[i].y2 );
             counts[i] = out.size();
          },
          1 );
      t_query_total.stop();
      size_t total = reduce( counts );

      cout << "ITER" << fixed << setprecision( 3 ) << ", algo="
           << "RangeTree"
           << ", name=" << benchmark_name << ", n=" << points.size()
           << ", q=" << query_num << ", p=" << num_workers()
           << ", min-val=" << min_val << ", max-val=" << max_val
           << ", win-mean=" << win << ", iteration=" << iteration
           << ", build-time=" << total_tm.get_total() << ", size_in_GiB="
           << ( static_cast<double>( r.size_in_bytes() ) / std::pow( 1024, 3 ) )
           << ", reserve-time=" << reserve_tm.get_total()
           << ", query-time=" << t_query_total.get_total()
           << ", total=" << total << endl;
      build_time = total_tm.get_total();
      size_in_gib =
          ( static_cast<double>( r.size_in_bytes() ) / std::pow( 1024, 3 ) );
      query_time = t_query_total.get_total();
   }
   reset_timers();
   RQ::finish();
   return std::make_tuple( build_time, size_in_gib, query_time );
}

auto
run_sum( sequence<point_type>& points, size_t iteration, data_type min_val,
         data_type max_val, size_t query_num )
{
   string benchmark_name = "Query-Sum";
   double build_time;
   double size_in_gib;
   double query_time;
   {
      RQ r( points );
      r.print_status();
      sequence<Query_data> queries =
          generate_queries( query_num, min_val, max_val );

      sequence<size_t> counts( query_num );

      timer t_query_total;
      t_query_total.start();
      parallel_for( 0, query_num,
                    [&]( size_t i )
                    {
                       counts[i] = r.query_sum( queries[i].x1, queries[i].y1,
                                                queries[i].x2, queries[i].y2 );
                    } );

      t_query_total.stop();
      size_t total = reduce( counts );

      cout << "ITER" << fixed << setprecision( 3 ) << ", algo="
           << "RangeTree"
           << ", name=" << benchmark_name << ", n=" << points.size()
           << ", q=" << query_num << ", p=" << num_workers()
           << ", min-val=" << min_val << ", max-val=" << max_val
           << ", win-mean=" << win << ", iteration=" << iteration
           << ", build-time=" << total_tm.get_total() << ", size_in_GiB="
           << ( static_cast<double>( r.size_in_bytes() ) / std::pow( 1024, 3 ) )
           << ", reserve-time=" << reserve_tm.get_total()
           << ", query-time=" << t_query_total.get_total()
           << ", total=" << total << endl;

      build_time = total_tm.get_total();
      size_in_gib =
          ( static_cast<double>( r.size_in_bytes() ) / std::pow( 1024, 3 ) );
      query_time = t_query_total.get_total();
   }
   reset_timers();
   RQ::finish();
   return std::make_tuple( build_time, size_in_gib, query_time );
}

void
run_insert( sequence<point_type>& points, size_t iteration, data_type min_val,
            data_type max_val, size_t query_num )
{
   string benchmark_name = "Query-Insert";
   {
      RQ r( points );

      sequence<point_type> new_points = generate_points(
          query_num, min_val, insert_range, 'i' + 'n' + 's' + 'e' + 'r' + 't' );
      for( size_t i = 0; i < query_num; i++ )
         new_points[i].y =
             random_hash( 2991, i, 0, 1000000000 ) + new_points[i].y;
      cout << "constructed" << endl;
      r.print_status();
      timer t_query_total;
      t_query_total.start();
      for( size_t i = 0; i < query_num; i++ )
      {
         r.insert_point( new_points[i] );
      }
      cout << "query end" << endl;
      t_query_total.stop();
      // r.print_status();
      cout << "RESULT" << fixed << setprecision( 3 ) << ", algo="
           << "RangeTree"
           << ", name=" << benchmark_name << ", n=" << points.size()
           << ", q=" << query_num << ", p=" << num_workers()
           << ", min-val=" << min_val << ", max-val=" << max_val
           << ", win-mean=" << win << ", iteration=" << iteration
           << ", build-time=" << total_tm.get_total() << ", size_in_GiB="
           << ( static_cast<double>( r.size_in_bytes() ) / std::pow( 1024, 3 ) )
           << ", reserve-time=" << reserve_tm.get_total()
           << ", query-time=" << t_query_total.get_total() << endl;
   }
   reset_timers();
   RQ::finish();
}

void
run_insert_lazy( sequence<point_type>& points, size_t iteration,
                 data_type min_val, data_type max_val, size_t query_num )
{
   string benchmark_name = "Query-Insert (Lazy)";
   {
      RQ r( points );

      sequence<point_type> new_points = generate_points(
          query_num, min_val, insert_range, 'i' + 'n' + 's' + 'e' + 'r' + 't' );
      for( size_t i = 0; i < query_num; i++ )
         new_points[i].y =
             random_hash( 2991, i, 0, 1000000000 ) + new_points[i].y;
      r.print_status();
      timer t_query_total;
      t_query_total.start();
      for( size_t i = 0; i < query_num; i++ )
      {
         r.insert_point_lazy( new_points[i] );
      }
      cout << endl;
      cout << "query end" << endl;
      t_query_total.stop();
      cout << "RESULT" << fixed << setprecision( 3 ) << ", algo="
           << "RangeTree"
           << ", name=" << benchmark_name << ", n=" << points.size()
           << ", q=" << query_num << ", p=" << num_workers()
           << ", min-val=" << min_val << ", max-val=" << max_val
           << ", win-mean=" << win << ", iteration=" << iteration
           << ", build-time=" << total_tm.get_total() << ", size_in_GiB="
           << ( static_cast<double>( r.size_in_bytes() ) / std::pow( 1024, 3 ) )
           << ", reserve-time=" << reserve_tm.get_total()
           << ", query-time=" << t_query_total.get_total() << endl;
   }
   reset_timers();
   RQ::finish();
}

void
test_inserts( size_t n, int min_val, int max_val, size_t iterations,
              size_t query_num, int type )
{
   for( size_t i = 0; i < iterations; ++i )
   {
      sequence<point_type> points = generate_points( n, min_val, max_val );
      if( type == 2 )
         run_insert( points, i, min_val, max_val, query_num );
      else
         run_insert_lazy( points, i, min_val, max_val, query_num );
   }
}

void
test_loop( size_t n, int min_val, int max_val, size_t iterations,
           size_t query_num, int type )
{
   sequence<double> build_tm( iterations );
   sequence<double> query_tm( iterations );
   sequence<double> space_usage( iterations );
   for( size_t i = 0; i < iterations; ++i )
   {
      sequence<point_type> points = generate_points( n, min_val, max_val );
      if( type == 0 )
      {
         auto [build, size, query] =
             run_all( points, i, min_val, max_val, query_num );
         build_tm[i] = build;
         query_tm[i] = query;
         space_usage[i] = size;
      }
      else
      {
         auto [build, size, query] =
             run_sum( points, i, min_val, max_val, query_num );
         build_tm[i] = build;
         query_tm[i] = query;
         space_usage[i] = size;
      }
   }

   auto less = []( double a, double b ) { return a < b; };
   parlay::sort( parlay::make_slice( build_tm ), less );
   parlay::sort( parlay::make_slice( query_tm ), less );
   parlay::sort( parlay::make_slice( space_usage ), less );

   cout << "RESULT" << fixed << setprecision( 3 ) << ", algo = "
        << "RangeTree"
        << ", name = "
        << "range build"
        << ", n = " << n << ", threads = " << parlay::num_workers()
        << ", rounds = " << iterations << ", q = " << query_num
        << ", time = " << build_tm[iterations / 2]
        << ", size_in_GiB = " << space_usage[iterations - 1] << endl;

   cout << "RESULT" << fixed << setprecision( 3 ) << ", algo = "
        << "RangeTree"
        << ", name = " << ( ( type == 0 ) ? "query-all" : "query-sum" )
        << ", n = " << n << ", threads = " << parlay::num_workers()
        << ", rounds = " << iterations << ", q = " << query_num
        << ", time = " << query_tm[iterations / 2]
        << ", size_in_GiB = " << space_usage[iterations - 1] << endl;
}

void
range_query( const points& wp, const points& wi, int N, int rounds,
             int queryType, int tag )
{
   RQ r;

   double ave_build = time_loop(
       rounds, 1.0, [&]() {}, [&]() { r = std::move( RQ( wp ) ); },
       [&]() { r.clear(); } );

   r = RQ( wp );
   std::cout << ave_build << " " << std::flush;

   if( tag >= 1 )
   {
      assert( wi.size() );
      double ave_insert = time_loop(
          1, 0.0, [&]() {},
          [&]()
          {
             for( size_t i = 0; i < wi.size(); i++ )
             {
                r.insert_point( wi[i] );
             }
          },
          [&]() {} );
      std::cout << ave_insert << " " << std::flush;
   }
   else
   {
      std::cout << "-1 " << std::flush;
   }

   if( tag >= 2 )
   {
      assert( wi.size() );
      double ave_delete = time_loop(
          1, 0.0, [&]() {},
          [&]()
          {
             for( size_t i = 0; i < wi.size(); i++ )
             {
                r.delete_point( wi[i] );
             }
          },
          [&]() {} );
      std::cout << ave_delete << " " << std::flush;
   }
   else
   {
      std::cout << "-1 " << std::flush;
   }

   std::cout << "-1 -1 -1 " << std::flush;

   int queryNum = 1000;
   sequence<Query_data> queries = gen_quires_sum( wp, queryNum );
   sequence<size_t> counts( queryNum );

   if( queryType & ( 1 << 1 ) )
   {
      double ave_count = time_loop(
          rounds, 1.0, [&]() {},
          [&]()
          {
             for( int i = 0; i < queryNum; i++ )
             {
                counts[i] = r.query_sum( queries[i].x1, queries[i].y1,
                                         queries[i].x2, queries[i].y2 );
             }
          },
          [&]() {} );
      std::cout << ave_count << " " << std::flush;
   }
   else
   {
      std::cout << "-1 " << std::flush;
   }

   if( queryType & ( 1 << 2 ) )
   {
      double ave_query = time_loop(
          rounds, 1.0, [&]() {},
          [&]()
          {
             for( int i = 0; i < queryNum; i++ )
             {
                sequence<pair<int, int>> out =
                    r.query_range( queries[i].x1, queries[i].y1, queries[i].x2,
                                   queries[i].y2 );
                counts[i] = out.size();
             }
          },
          [&]() {} );
      std::cout << ave_query << std::endl << std::flush;
   }
   else
   {
      std::cout << "-1 " << std::flush;
   }

   return;
}

int
main( int argc, char** argv )
{
   // n: input size
   // coordinates in range [l, h]
   // run in r rounds and q queries
   // dist = 0  means random query windows
   // dist != 0 means average query window edge length of w/2
   // query_type: 0 for report-all, 1 for report-sum, 2 for insert, 3 for lazy
   // insert insert_range: the coordinate range of insertions
   // if( argc == 1 )
   // {
   //    cout << "./rt_test [-n size] [-l rmin] [-h rmax] [-r rounds] [-q "
   //            "queries] [-d dist] [-w window] [-t query_type] [-e
   //            insert_range]"
   //         << endl;
   //    cout << "n: input size" << endl;
   //    cout << "coordinates in range [l, h]" << endl;
   //    cout << "run in r rounds and q queries" << endl;
   //    cout << "dist = 0  means random query windows" << endl;
   //    cout << "dist != 0 means average query window edge length of w/2" <<
   //    endl; cout << "query_type: 0 for report-all, 1 for report-sum, 2 for
   //    insert, 3 "
   //            "for lazy insert"
   //         << endl;
   //    cout << "insert_range: the coordinate range of insertions" << endl;
   // }
   // commandLine P(
   //     argc, argv,
   //     "./rt_test [-n size] [-l rmin] [-h rmax] [-r rounds] [-q queries] [-d
   //     " "dist] [-w window] [-t query_type] [-e insert_range]" );
   // size_t n = P.getOptionLongValue( "-n", 100000000 );
   // int min_val = P.getOptionIntValue( "-l", 0 );
   // int max_val = P.getOptionIntValue( "-h", 1000000000 );
   // size_t iterations = P.getOptionIntValue( "-r", 3 );
   // dist = P.getOptionIntValue( "-d", 0 );
   // win = P.getOptionIntValue( "-w", 1000000 );
   // size_t query_num = P.getOptionLongValue( "-q", 1000 );
   // int type = P.getOptionIntValue( "-t", 0 );
   // insert_range = P.getOptionIntValue( "-e", max_val );
   // srand( 2017 );
   // test_loop( n, min_val, max_val, iterations, query_num, type );

   commandLine P( argc, argv,
                  "[-k {1,...,100}] [-d {2,3,5,7,9,10}] [-n <node num>] [-t "
                  "<parallelTag>] [-p <inFile>] [-r {1,...,5}] [-q {0,1}] [-i "
                  "<_insertFile>]" );
   char* iFile = P.getOptionValue( "-p" );
   char* _insertFile = P.getOptionValue( "-i" );
   int K = P.getOptionIntValue( "-k", 100 );
   int Dim = P.getOptionIntValue( "-d", 3 );
   size_t N = P.getOptionLongValue( "-n", -1 );
   int tag = P.getOptionIntValue( "-t", 1 );
   int rounds = P.getOptionIntValue( "-r", 3 );
   int queryType = P.getOptionIntValue( "-q", 0 );

   std::string name, insertFile;
   points wp;
   points win;

   //* initialize wp
   if( iFile != NULL )
   {
      name = std::string( iFile );
      name = name.substr( name.rfind( "/" ) + 1 );
      std::cout << name << " ";
      wp = read_points( iFile );
      N = wp.size();
   }

   if( tag >= 1 )
   {
      if( _insertFile == NULL )
      {
         int id = std::stoi( name.substr( 0, name.find_first_of( '.' ) ) );
         id = ( id + 1 ) % 3;  //! MOD graph number used to test
         if( !id ) id++;
         int pos = std::string( iFile ).rfind( "/" ) + 1;
         insertFile = std::string( iFile ).substr( 0, pos ) +
                      std::to_string( id ) + ".in";
      }
      else
      {
         insertFile = std::string( _insertFile );
      }

      win = read_points( insertFile.c_str() );
   }

   wp = parlay::unique( parlay::sort( wp ),
                        [&]( const point_type& a, const point_type& b )
                        { return a == b; } );
   win = parlay::unique( parlay::sort( win ),
                         [&]( const point_type& a, const point_type& b )
                         { return a == b; } );

   range_query( wp, win, N, rounds, queryType, tag );

   return 0;
}
