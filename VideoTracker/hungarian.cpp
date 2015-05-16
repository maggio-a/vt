
/**
 * Code brutally adapted from the C implementation by Mattias Andrée
 * available at https://github.com/maandree/hungarian-algorithm-n3
 * (basically just changed malloc/free to new/delete and added the interface method)
 *
 */
#include "hungarian.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>
#include <opencv2/core/core.hpp>


using namespace std;

#ifndef RANDOM_DEVICE
#define RANDOM_DEVICE "/dev/urandom"
#endif

#define cell      long
#define CELL_STR  "%li"

#define llong    int_fast64_t
#define byte     int_fast8_t
#define boolean  int_fast8_t
#define null     0
#define true     1
#define false    0

#ifdef DEBUG
#  define debug(X) fprintf(stderr, "\033[31m%s\033[m\n", X)
#else
#  define debug(X) 
#endif

/**
 * Cell marking:  none
 */
#define UNMARKED  0L

/**
 * Cell marking:  marked
 */
#define MARKED    1L

/**
 * Cell marking:  prime
 */
#define PRIME     2L


/**
 * Bit set, a set of fixed number of bits/booleans
 */
typedef struct
{
	/**
	 * The set of all limbs, a limb consist of 64 bits
	 */
	llong* limbs;
	
	/**
	 * Singleton array with the index of the first non-zero limb
	 */
	size_t* first;
	
	/**
	 * Array the the index of the previous non-zero limb for each limb
	 */
	size_t* prev;
	
	/**
	 * Array the the index of the next non-zero limb for each limb
	 */
	size_t* next;
	
} BitSet;

ssize_t** kuhn_match(cell** table, size_t n, size_t m);
void kuhn_reduceRows(cell** t, size_t n, size_t m);
byte** kuhn_mark(cell** t, size_t n, size_t m);
boolean kuhn_isDone(byte** marks, boolean* colCovered, size_t n, size_t m);
size_t* kuhn_findPrime(cell** t, byte** marks, boolean* rowCovered, boolean* colCovered, size_t n, size_t m);
void kuhn_altMarks(byte** marks, size_t* altRow, size_t* altCol, ssize_t* colMarks, ssize_t* rowPrimes, size_t* prime, size_t n, size_t m);
void kuhn_addAndSubtract(cell** t, boolean* rowCovered, boolean* colCovered, size_t n, size_t m);
ssize_t** kuhn_assign(byte** marks, size_t n, size_t m);

BitSet new_BitSet(size_t size);
void BitSet_set(BitSet this_obj, size_t i);
void BitSet_unset(BitSet this_obj, size_t i);
ssize_t BitSet_any(BitSet this_obj) __attribute__((pure));

size_t lb(llong x) __attribute__((const));

void print_asd(cell** t, size_t n, size_t m, ssize_t** assignment);

long dist(cv::Point2f p1, cv::Point2f p2) {
	cv::Point2f diff = p1 - p2;
	return std::sqrt(diff.x*diff.x + diff.y*diff.y);
}

//FIXME 1000
vector<size_t> ComputeMatching(vector<cv::Point2f> predictions, vector<cv::Point2f> detections) {
	assert(predictions.size() > 0 && detections.size() > 0);
	size_t n = predictions.size();
	size_t m = std::max(predictions.size(), detections.size());
	cell** table = new cell*[n];
	for (size_t i = 0; i < n; i++) {
		*(table + i) = new cell[m];
		for (size_t j = 0; j < m; j++) {
			if (j < detections.size())
				*(*(table + i) + j) = cell(dist(predictions[i], detections[j]));
			else
				*(*(table + i) + j) = 1000;
		}
	}
	//cout << "\nMatching table:\n";
	//print_asd(table, n, m, 0);
	ssize_t **assignment = kuhn_match(table, n, m);
	vector<size_t> assign(n, m); //initialize to m (invalid prediction index)
	for (size_t i = 0; i < n; ++i) {
		size_t ipred = *(*(assignment + i));
		size_t idet = *(*(assignment + i) + 1);
		// ensure safe conversion from ssize_t
        assert (ipred < n && idet < m);
		assign[ipred] = idet;
		delete[] *(assignment + i);
		delete[] *(table + i);
	}
	delete[] assignment;
	delete[] table;
	// assert every prediction got an assignment
	for (size_t i = 0; i < n; ++i) {
		assert(assign[i] != m);
	}

	return assign;
}

/*
int main(int argc, char** argv)
{
	vector<Point2i> pred;
	vector<Point2i> det;

	pred.push_back(Point2i(10, 10));
	pred.push_back(Point2i(13, 15));
	pred.push_back(Point2i(18, 32));

	det.push_back(Point2i(5, 16));
	det.push_back(Point2i(20, 6));
	det.push_back(Point2i(11, 11));
	det.push_back(Point2i(11, 12));



	vector<size_t> match = computeMatching(pred, det);
	for (size_t i = 0; i < match.size(); ++i) {
		std::cout << "(" << i << "," << match[i] << ") " << std::endl;
	}
	cout << sizeof(ssize_t) << " " << sizeof(size_t) << endl;
	cout << sizeof(long) << endl;
	return 0;
}*/

void print_asd(cell** t, size_t n, size_t m, ssize_t** assignment)
{
	size_t i, j;
	
	ssize_t** assigned = new ssize_t*[n];
	for (i = 0; i < n; i++)
	{
		*(assigned + i) = new ssize_t[m];
	for (j = 0; j < m; j++)
		*(*(assigned + i) + j) = 0;
	}
	if (assignment != null)
		for (i = 0; i < n; i++)
		(*(*(assigned + **(assignment + i)) + *(*(assignment + i) + 1)))++;
	
	for (i = 0; i < n; i++)
	{
	printf("    ");
	for (j = 0; j < m; j++)
	{
		if (*(*(assigned + i) + j))
		  printf("\033[%im", (int)(30 + *(*(assigned + i) + j)));
		printf("%5li%s\033[m   ", (cell)(*(*(t + i) + j)), (*(*(assigned + i) + j) ? "^" : " "));
		}
	printf("\n\n");
	
	delete[] (*(assigned + i));
	}
	
	delete[] assigned;
}



/**
 * Calculates an optimal bipartite minimum weight matching using an
 * O(n³)-time implementation of The Hungarian Algorithm, also known
 * as Kuhn's Algorithm.
 * 
 * @param   table  The table in which to perform the matching
 * @param   n      The height of the table
 * @param   m      The width of the table
 * @return         The optimal assignment, an array of row–coloumn pairs
 */
ssize_t** kuhn_match(cell** table, size_t n, size_t m)
{
	size_t i;
	
	/* not copying table since it will only be used once */
	
	kuhn_reduceRows(table, n, m);
	byte** marks = kuhn_mark(table, n, m);
	
	boolean* rowCovered = new boolean[n];
	boolean* colCovered = new boolean[m];
	for (i = 0; i < n; i++)
	{
		*(rowCovered + i) = false;
		*(colCovered + i) = false;
	}
	for (i = n; i < m; i++)
		*(colCovered + i) = false;
	
	size_t* altRow = new size_t[m*n];
	size_t* altCol = new size_t[m*n];
	
	ssize_t* rowPrimes = new ssize_t[n];
	ssize_t* colMarks  = new ssize_t [m];
	
	size_t* prime;
	
	for (;;)
	{
	if (kuhn_isDone(marks, colCovered, n, m))
		break;
	
		for (;;)
	{
		prime = kuhn_findPrime(table, marks, rowCovered, colCovered, n, m);
		if (prime != null)
		{
		kuhn_altMarks(marks, altRow, altCol, colMarks, rowPrimes, prime, n, m);
		for (i = 0; i < n; i++)
		{
			*(rowCovered + i) = false;
			*(colCovered + i) = false;
		}
		for (i = n; i < m; i++)
			*(colCovered + i) = false;
		delete[] prime;
		break;
		}
		kuhn_addAndSubtract(table, rowCovered, colCovered, n, m);
	}
	}
	
	delete[] rowCovered;
	delete[] colCovered;
	delete[] altRow;
	delete[] altCol;
	delete[] rowPrimes;
	delete[] colMarks;
	
	ssize_t** rc = kuhn_assign(marks, n, m);
	
	for (i = 0; i < n; i++)
		delete[] *(marks + i);
	delete[] marks;
	
	return rc;
}


/**
 * Reduces the values on each rows so that, for each row, the
 * lowest cells value is zero, and all cells' values is decrease
 * with the same value [the minium value in the row].
 * 
 * @param  t  The table in which to perform the reduction
 * @param  n  The table's height
 * @param  m  The table's width
 */
void kuhn_reduceRows(cell** t, size_t n, size_t m)
{
	size_t i, j;
	cell min;
	cell* ti;
	for (i = 0; i < n; i++)
	{
		ti = *(t + i);
		min = *ti;
	for (j = 1; j < m; j++)
		if (min > *(ti + j))
			min = *(ti + j);
	
	for (j = 0; j < m; j++)
		*(ti + j) -= min;
	}
}


/**
 * Create a matrix with marking of cells in the table whose
 * value is zero [minimal for the row]. Each marking will
 * be on an unique row and an unique column.
 * 
 * @param   t  The table in which to perform the reduction
 * @param   n  The table's height
 * @param   m  The table's width
 * @return     A matrix of markings as described in the summary
 */
byte** kuhn_mark(cell** t, size_t n, size_t m)
{
	size_t i, j;
	byte** marks = new byte*[n];
	byte* marksi;
	for (i = 0; i < n; i++)
	{
	  marksi = *(marks + i) = new byte[m];
		for (j = 0; j < m; j++)
		*(marksi + j) = UNMARKED;
	}
	
	boolean* rowCovered = new boolean[n];
	boolean* colCovered = new boolean[m];
	for (i = 0; i < n; i++)
	{
		*(rowCovered + i) = false;
		*(colCovered + i) = false;
	}
	for (i = 0; i < m; i++)
		*(colCovered + i) = false;
	
	for (i = 0; i < n; i++)
		for (j = 0; j < m; j++)
		if ((!*(rowCovered + i)) && (!*(colCovered + j)) && (*(*(t + i) + j) == 0))
		{
			*(*(marks + i) + j) = MARKED;
		*(rowCovered + i) = true;
		*(colCovered + j) = true;
		}
	
	delete[] rowCovered;
	delete[] colCovered;
	return marks;
}


/**
 * Determines whether the marking is complete, that is
 * if each row has a marking which is on a unique column.
 *
 * @param   marks       The marking matrix
 * @param   colCovered  An array which tells whether a column is covered
 * @param   n           The table's height
 * @param   m           The table's width
 * @return              Whether the marking is complete
 */
boolean kuhn_isDone(byte** marks, boolean* colCovered, size_t n, size_t m)
{
	size_t i, j;
	for (j = 0; j < m; j++)
		for (i = 0; i < n; i++)
		if (*(*(marks + i) + j) == MARKED)
		{
			*(colCovered + j) = true;
		break;
		}
	
	size_t count = 0;
	for (j = 0; j < m; j++)
		if (*(colCovered + j))
		count++;
	
	return count == n;
}


/**
 * Finds a prime
 * 
 * @param   t           The table
 * @param   marks       The marking matrix
 * @param   rowCovered  Row cover array
 * @param   colCovered  Column cover array
 * @param   n           The table's height
 * @param   m           The table's width
 * @return              The row and column of the found print, <code>null</code> will be returned if none can be found
 */
size_t* kuhn_findPrime(cell** t, byte** marks, boolean* rowCovered, boolean* colCovered, size_t n, size_t m)
{
	size_t i, j;
	BitSet zeroes = new_BitSet(n * m);
	
	for (i = 0; i < n; i++)
		if (!*(rowCovered + i))
		for (j = 0; j < m; j++)
			if ((!*(colCovered + j)) && (*(*(t + i) + j) == 0))
		  BitSet_set(zeroes, i * m + j);
	
	ssize_t p;
	size_t row, col;
	boolean markInRow;
	
	for (;;)
	{
		p = BitSet_any(zeroes);
	if (p < 0)
		{
		delete[] zeroes.limbs;
		delete[] zeroes.first;
		delete[] zeroes.next;
		delete[] zeroes.prev;
		return null;
	}
	
	row = (size_t)p / m;
	col = (size_t)p % m;
	
	*(*(marks + row) + col) = PRIME;
	
	markInRow = false;
	for (j = 0; j < m; j++)
		if (*(*(marks + row) + j) == MARKED)
		{
		markInRow = true;
		col = j;
		}
	
	if (markInRow)
	{
		*(rowCovered + row) = true;
		*(colCovered + col) = false;
		
		for (i = 0; i < n; i++)
			if ((*(*(t + i) + col) == 0) && (row != i))
		{
			if ((!*(rowCovered + i)) && (!*(colCovered + col)))
				BitSet_set(zeroes, i * m + col);
			else
				BitSet_unset(zeroes, i * m + col);
		}
		
		for (j = 0; j < m; j++)
			if ((*(*(t + row) + j) == 0) && (col != j))
		{
			if ((!*(rowCovered + row)) && (!*(colCovered + j)))
				BitSet_set(zeroes, row * m + j);
			else
				BitSet_unset(zeroes, row * m + j);
		}
		
		if ((!*(rowCovered + row)) && (!*(colCovered + col)))
			BitSet_set(zeroes, row * m + col);
		else
			BitSet_unset(zeroes, row * m + col);
	}
	else
	{
		size_t* rc = new size_t[2];
		*rc = row;
		*(rc + 1) = col;
		delete[] zeroes.limbs;
		delete[] zeroes.first;
		delete[] zeroes.next;
		delete[] zeroes.prev;
		return rc;
	}
	}
}


/**
 * Removes all prime marks and modifies the marking
 *
 * @param  marks      The marking matrix
 * @param  altRow     Marking modification path rows
 * @param  altCol     Marking modification path columns
 * @param  colMarks   Markings in the columns
 * @param  rowPrimes  Primes in the rows
 * @param  prime      The last found prime
 * @param  n          The table's height
 * @param  m          The table's width
 */
void kuhn_altMarks(byte** marks, size_t* altRow, size_t* altCol, ssize_t* colMarks, ssize_t* rowPrimes, size_t* prime, size_t n, size_t m)
{
	size_t index = 0, i, j;
	*altRow = *prime;
	*altCol = *(prime + 1);
	
	for (i = 0; i < n; i++)
	{
		*(rowPrimes + i) = -1;
		*(colMarks + i) = -1;
	}
	for (i = n; i < m; i++)
		*(colMarks + i) = -1;
	
	for (i = 0; i < n; i++)
		for (j = 0; j < m; j++)
		if (*(*(marks + i) + j) == MARKED)
			*(colMarks + j) = (ssize_t)i;
		else if (*(*(marks + i) + j) == PRIME)
			*(rowPrimes + i) = (ssize_t)j;
	
	ssize_t row, col;
	for (;;)
	{
		row = *(colMarks + *(altCol + index));
	if (row < 0)
		break;
	
	index++;
	*(altRow + index) = (size_t)row;
	*(altCol + index) = *(altCol + index - 1);
	
	col = *(rowPrimes + *(altRow + index));
	
	index++;
	*(altRow + index) = *(altRow + index - 1);
	*(altCol + index) = (size_t)col;
	}
	
	byte* markx;
	for (i = 0; i <= index; i++)
	{
		markx = *(marks + *(altRow + i)) + *(altCol + i);
		if (*markx == MARKED)
		*markx = UNMARKED;
	else
		*markx = MARKED;
	}
	
	byte* marksi;
	for (i = 0; i < n; i++)
	{
		marksi = *(marks + i);
		for (j = 0; j < m; j++)
		if (*(marksi + j) == PRIME)
			*(marksi + j) = UNMARKED;
	}
}


/**
 * Depending on whether the cells' rows and columns are covered,
 * the the minimum value in the table is added, subtracted or
 * neither from the cells.
 *
 * @param  t           The table to manipulate
 * @param  rowCovered  Array that tell whether the rows are covered
 * @param  colCovered  Array that tell whether the columns are covered
 * @param  n           The table's height
 * @param  m           The table's width
 */
void kuhn_addAndSubtract(cell** t, boolean* rowCovered, boolean* colCovered, size_t n, size_t m)
{
	size_t i, j;
	cell min = 0x7FFFffffL;
	for (i = 0; i < n; i++)
		if (!*(rowCovered + i))
		for (j = 0; j < m; j++)
			if ((!*(colCovered + j)) && (min > *(*(t + i) + j)))
			min = *(*(t + i) + j);
	
	for (i = 0; i < n; i++)
		for (j = 0; j < m; j++)
	{
		if (*(rowCovered + i))
			*(*(t + i) + j) += min;
		if (*(colCovered + j) == false)
			*(*(t + i) + j) -= min;
	}
}


/**
 * Creates a list of the assignment cells
 * 
 * @param   marks  Matrix markings
 * @param   n      The table's height
 * @param   m      The table's width
 * @return         The assignment, an array of row–coloumn pairs
 */
ssize_t** kuhn_assign(byte** marks, size_t n, size_t m)
{
	ssize_t** assignment = new ssize_t*[n];
	
	size_t i, j;
	for (i = 0; i < n; i++)
	{
		*(assignment + i) = new ssize_t[2];
		for (j = 0; j < m; j++)
		if (*(*(marks + i) + j) == MARKED)
		{
		**(assignment + i) = (ssize_t)i;
		*(*(assignment + i) + 1) = (ssize_t)j;
		}
	}
	
	return assignment;
}


/**
 * Constructor for BitSet
 *
 * @param   size  The (fixed) number of bits to bit set should contain
 * @return        The a unique BitSet instance with the specified size
 */
BitSet new_BitSet(size_t size)
{
	BitSet this_obj;
	
	size_t c = size >> 6L;
	if (size & 63L)
		c++;
	
	this_obj.limbs = new llong[c];
	this_obj.prev = new size_t[c+1];
	this_obj.next = new size_t[c+1];
	*(this_obj.first = new size_t) = 0;
	
	size_t i;
	for (i = 0; i < c; i++)
	{
		*(this_obj.limbs + i) = 0LL;
		*(this_obj.prev + i) = *(this_obj.next + i) = 0L;
	}
	*(this_obj.prev + c) = *(this_obj.next + c) = 0L;
	
	return this_obj;
}

/**
 * Turns on a bit in a bit set
 * 
 * @param  this_obj  The bit set
 * @param  i     The index of the bit to turn on
 */
void BitSet_set(BitSet this_obj, size_t i)
{
	size_t j = i >> 6L;
	llong old = *(this_obj.limbs + j);
	
	*(this_obj.limbs + j) |= 1LL << (llong)(i & 63L);
	
	if ((!*(this_obj.limbs + j)) ^ (!old))
	{
		j++;
	*(this_obj.prev + *(this_obj.first)) = j;
	*(this_obj.prev + j) = 0;
	*(this_obj.next + j) = *(this_obj.first);
	*(this_obj.first) = j;
	}
}

/**
 * Turns off a bit in a bit set
 * 
 * @param  this_obj  The bit set
 * @param  i     The index of the bit to turn off
 */
void BitSet_unset(BitSet this_obj, size_t i)
{
	size_t j = i >> 6L;
	llong old = *(this_obj.limbs + j);
	
	*(this_obj.limbs + j) &= ~(1LL << (llong)(i & 63L));
	
	if ((!*(this_obj.limbs + j)) ^ (!old))
	{
		j++;
	size_t p = *(this_obj.prev + j);
	size_t n = *(this_obj.next + j);
	*(this_obj.prev + n) = p;
	*(this_obj.next + p) = n;
	if (*(this_obj.first) == j)
		*(this_obj.first) = n;
	}
}

/**
 * Gets the index of any set bit in a bit set
 * 
 * @param   this_obj  The bit set
 * @return        The index of any set bit
 */
ssize_t BitSet_any(BitSet this_obj)
{
	if (*(this_obj.first) == 0L)
		return -1;
	
	size_t i = *(this_obj.first) - 1;
	return (ssize_t)(lb(*(this_obj.limbs + i) & -*(this_obj.limbs + i)) + (i << 6L));
}


/**
 * Calculates the floored binary logarithm of a positive integer
 *
 * @param   value  The integer whose logarithm to calculate
 * @return         The floored binary logarithm of the integer
 */
size_t lb(llong value)
{
	size_t rc = 0;
	llong v = value;
	
	if (v & (int_fast64_t)0xFFFFFFFF00000000LL)  {  rc |= 32L;  v >>= 32LL;  }
	if (v & (int_fast64_t)0x00000000FFFF0000LL)  {  rc |= 16L;  v >>= 16LL;  }
	if (v & (int_fast64_t)0x000000000000FF00LL)  {  rc |=  8L;  v >>=  8LL;  }
	if (v & (int_fast64_t)0x00000000000000F0LL)  {  rc |=  4L;  v >>=  4LL;  }
	if (v & (int_fast64_t)0x000000000000000CLL)  {  rc |=  2L;  v >>=  2LL;  }
	if (v & (int_fast64_t)0x0000000000000002LL)     rc |=  1L;
	
	return rc;
}

