using namespace std;

#include "../common/geometryIO.h"
#include "parlay/primitives.h"

using data_type = int;
using weight = float;

using parlay::num_workers;
using parlay::parallel_for;
using parlay::sequence;
using parlay::tabulate;
using std::cout;
using std::endl;

int insert_range;

template <typename c_type, typename w_type>
struct Point
{
   data_type x, y;
   weight w;
   Point() {}
   Point( data_type x, data_type y, weight w ) : x( x ), y( y ), w( w ) {}
   bool
   operator<( const Point& b ) const
   {
      return x == b.x ? y < b.y : x < b.x;
   }
   bool
   operator==( const Point& b ) const
   {
      return ( x == b.x ) && ( y == b.y );
   }
};

using point_type = Point<data_type, data_type>;

template <typename value_type>
inline bool
inRange( value_type x, value_type l, value_type r )
{
   return ( ( x >= l ) && ( x <= r ) );
}

struct Query_data
{
   data_type x1, x2, y1, y2;
} query_data;

int win;
int dist;

int
random_hash( int seed, int a, int rangeL, int rangeR )
{
   if( rangeL == rangeR ) return rangeL;
   a = ( a + seed ) + ( seed << 7 );
   a = ( a + 0x7ed55d16 ) + ( a << 12 );
   a = ( a ^ 0xc761c23c ) ^ ( a >> 19 );
   a = ( a + 0x165667b1 ) + ( a << 5 );
   a = ( ( a + seed ) >> 5 ) ^ ( a << 9 );
   a = ( a + 0xfd7046c5 ) + ( a << 3 );
   a = ( a ^ 0xb55a4f09 ) ^ ( a >> 16 );
   a = a % ( rangeR - rangeL );
   if( a < 0 ) a += ( rangeR - rangeL );
   a += rangeL;
   return a;
}

// generate random input points. The coordinate are uniformly distributed in [a,
// b]
sequence<point_type>
generate_points( size_t n, data_type a, data_type b, data_type offset = 0 )
{
   sequence<point_type> ret( n );
   parallel_for( 0, n,
                 [&]( size_t i )
                 {
                    ret[i].x = random_hash( 'x' + offset, i, a, b );
                    ret[i].y = random_hash( 'y' + offset, i, a, b );
                    ret[i].w = 1;  // random_hash('w', i, a, b);
                 } );

   return ret;
}

sequence<point_type>
read_points( const char* iFile )
{
   parlay::sequence<char> S = readStringFromFile( iFile );
   parlay::sequence<char*> W = stringToWords( S );
   size_t N = atol( W[0] );
   int Dim = atoi( W[1] );
   assert( N >= 0 && Dim == 2 );

   auto pts = W.cut( 2, W.size() );
   assert( pts.size() % Dim == 0 );
   size_t n = pts.size() / Dim;
   auto a = parlay::tabulate(
       Dim * n, [&]( size_t i ) -> data_type { return atol( pts[i] ); } );
   sequence<point_type> wp( N );
   parlay::parallel_for( 0, n,
                         [&]( size_t i )
                         {
                            wp[i].x = a[i * Dim];
                            wp[i].y = a[i * Dim + 1];
                            wp[i].w = 1;
                         } );
   return std::move( wp );
}

sequence<Query_data>
gen_quires_sum( const sequence<point_type>& wp, int queryNum )
{
   sequence<Query_data> ret( queryNum );

   int n = wp.size();
   parlay::parallel_for( 0, queryNum,
                         [&]( size_t i )
                         {
                            ret[i] =
                                Query_data{ wp[i].x, wp[( i + n / 2 ) % n].x,
                                            wp[i].y, wp[( i + n / 2 ) % n].y };
                         } );
   return std::move( ret );
}

// generate random query windows:
// if dist == 0: the coordinate of the query windows are uniformly random in [a,
// b] otherwise: the average size of query windows is win^2/4
sequence<Query_data>
generate_queries( size_t q, data_type a, data_type b )
{
   sequence<Query_data> ret( q );

   parallel_for( 0, q,
                 [&]( size_t i )
                 {
                    data_type x1 = random_hash( 'q' * 'x', i * 2, a, b );
                    data_type y1 = random_hash( 'q' * 'y', i * 2, a, b );
                    int dx = random_hash( 'd' * 'x', i, 0, win );
                    int dy = random_hash( 'd' * 'y', i, 0, win );
                    int x2 = x1 + dx;
                    int y2 = y1 + dy;
                    if( dist == 0 )
                    {
                       x2 = random_hash( 'q' * 'x', i * 2 + 1, a, b );
                       y2 = random_hash( 'q' * 'y', i * 2 + 1, a, b );
                    }
                    if( x1 > x2 )
                    {
                       data_type t = x1;
                       x1 = x2;
                       x2 = t;
                    }
                    if( y1 > y2 )
                    {
                       data_type t = y1;
                       y1 = y2;
                       y2 = t;
                    }
                    ret[i].x1 = x1;
                    ret[i].x2 = x2;
                    ret[i].y1 = y1;
                    ret[i].y2 = y2;
                 } );

   return ret;
}