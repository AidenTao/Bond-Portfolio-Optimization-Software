// The input file has various bonds with their present value, maturity, followed by their cash flows. 
//The target goal for this project is to Pick the bond-portfolio that will meet this obligation when it is due, and has the largest convexity among all possible portfolios choices. 
//Based on the dataset, the duration and convexity for each bond were easily to get
//According to proving, I got that the convexity of the portfolio is the (convex) combination of the convexity of each individual cash-flow. So did duration.

#include <iostream>
#include <cmath>
#include <vector>
#include <fstream>
#include <iomanip>
#include <cstdlib>

// include lp_solve library which is very convenient to address linear programming problems
#include "lp_lib.h"
#pragma comment (lib, "liblpsolve55.lib")
using namespace std;

const double Error = 1e-10;
int number_of_cash_flows;
vector <double> price_list;
vector <int> maturity_list;
 vector<double>cash_flow;
vector <double> yield_to_maturity;
vector <double> duration;
vector <double> convexity;
string fileName;
double debt_obligation_amount;
double time_when_debt_is_due;
vector <double> percentage_of_cash_flow_to_meet_debt_obligation;

double r_function(vector <double> cash_flow, double price, int maturity, double rate)
{
	// write a function that computes f(r)
	// f(r) is created to construct the YTM of the bond
	//Note that in this equation,  all cash flows are discounted at the same rate, the ytm that we are trying to solve
	double result;
	result = price * pow((1 + rate), maturity);
	for (int i = 0; i < maturity; i++) {
		result = result - cash_flow[i] * pow((1 + rate), maturity - 1 - i);
	}
	return result;
}

double derivative_function(vector <double> cash_flow, double price, int maturity, double rate)
{
	// write a function that computes f'(r) which is an inportant element in Newton-Raphson method
	double result;
	result = maturity * price * pow((1 + rate), maturity - 1);
	for (int j = 0; j < maturity - 1; j++) {
		result = result - cash_flow[j] * ((double)maturity - 1 - j) * pow((1 + rate), maturity - 2 - j);
	}
	return result;
}

double Newton_Raphson(vector <double> cash_flow, double price, int maturity, double rate)
{
	// write a function that finds the (only) root of f(r)
	//  using Newton-Raphson method
	while (abs(r_function(cash_flow, price, maturity, rate)) > Error) {
		rate = rate - r_function(cash_flow, price, maturity, rate) / derivative_function(cash_flow, price, maturity, rate);
	}
	return rate;
}

double get_duration(vector <double> cash_flow, double price, int maturity, double rate)
{
	// write a function that computes the duration of a cash flow
	double duration = 0;
	for (int i = 0; i < maturity; i++) {
		duration = duration + ((double)i + 1) * cash_flow[i] / pow((1 + rate), i + 1);
	}
	duration = duration / price;
	return duration;
}

double get_convexity(vector <double> cash_flow, double price, int maturity, double rate)
{
	// write a function that computes the convexity of a cash flow
	double convexity = 0;
	for (int j = 0; j < maturity; j++) {
		convexity = convexity + (((double)j + 1) * ((double)j + 2) * cash_flow[j]) / pow((1 + rate), j + 3);
	}
	convexity = convexity / price;
	return convexity;
}

double present_value_of_debt()
{
	// compute PV of future debt obligation
	// using the average-value-of-the-YTMs 
	double average_r;
	double PV;
	double sum = 0;
	for (int m = 0; m < number_of_cash_flows; m++) 
		sum = sum + yield_to_maturity[m];
	average_r = sum / number_of_cash_flows;
	PV = debt_obligation_amount / pow((1 + average_r), time_when_debt_is_due);
	return PV;
}

void print_data(char* filename)
{
	cout << "Input File: " << filename << endl;
	cout << "We owe " << debt_obligation_amount << " in " << time_when_debt_is_due << " years" << endl;
	cout << "Number of Cash Flows: " << number_of_cash_flows << endl;
	cout << "Present value of the debt(using avarage value of the YTMS)" << present_value_of_debt() << endl;
	for (int i = 0; i < number_of_cash_flows; i++)
	{
		cout << "---------------------------" << endl;
		cout << "Cash Flow #" << i + 1 << endl;
		cout << "Price = " << price_list[i] << endl;
		cout << "Maturity = " << maturity_list[i] << endl;
		cout << "Yield to Maturity = " << yield_to_maturity[i] << endl;
		cout << "Duration = " << duration[i] << endl;
		cout << "Convexity = " << convexity[i] << endl;
		cout << "Percentage of Face Value that would meet the obligation = " <<percentage_of_cash_flow_to_meet_debt_obligation[i] << endl;
	}
	cout << "---------------------------" << endl;
}

void get_data(char* argv[])
{
	// write the code that reads the data from the file identified 
	// on the command-line. 
	//cout << "Input File Name: " << endl;
	//cin >> fileName;
	ifstream input_filename(argv[1]);
	if (input_filename.is_open()) {
		double a;
		int b;
		double c;
		input_filename >> number_of_cash_flows;
		for (int i = 1; i <= number_of_cash_flows; i++) {
			input_filename >> a;
			price_list.push_back(a);
			input_filename >> b;
			maturity_list.push_back(b);
			for (int j = 1; j <= maturity_list[i - 1]; j++)
			{
				input_filename >> c;
				cash_flow.push_back(c);
			}
			double r;
			r = Newton_Raphson(cash_flow, price_list[i - 1], maturity_list[i - 1], 0);
			yield_to_maturity.push_back(r);
			a = get_duration(cash_flow, price_list[i - 1], maturity_list[i - 1], r);
			duration.push_back(a);
			c = get_convexity(cash_flow, price_list[i - 1], maturity_list[i - 1], r);
			convexity.push_back(c);
			cash_flow.clear();
		}
		input_filename >> debt_obligation_amount;
		input_filename >> time_when_debt_is_due;

		for (int n = 0; n < number_of_cash_flows; n++) {
			percentage_of_cash_flow_to_meet_debt_obligation.push_back(present_value_of_debt() / price_list[n]);
		}
	}
}

void get_optimal_portfolio()
{
	// write the lp_solve specific material that 
	// computes the optimal_portfolio
	lprec* lp;
	REAL* solution;
	solution= (double*)malloc(number_of_cash_flows * sizeof(double));

	// setting the problem up: number_of_cash_flows real variables
	lp = make_lp(0, number_of_cash_flows);
	// This keeps the message reporting of lp_solve to a minimum
	set_verbose(lp, 3);
	{   
		double* row;
		double A = (double)number_of_cash_flows + 1;
		row = (double*)malloc(A * sizeof(double));
		for (int i = 0; i < number_of_cash_flows; i++)
			row[i + 1] = (double)duration[i];
		row[0] = 0;
		add_constraint(lp,row, EQ, time_when_debt_is_due);
	}
	{
		double* row;
		double A = (double)number_of_cash_flows + 1;
		row = (double*)malloc(A * sizeof(double));
		for (int i = 0; i < number_of_cash_flows; i++)
			row[i + 1] = 1;
		row[0] = 0;
		add_constraint(lp, row, EQ, 1);
	}
	{
		double* row;
		double A = (double)number_of_cash_flows + 1;
		row = (double*)malloc(A * sizeof(double));
		for (int i = 0; i < number_of_cash_flows; i++)
			row[i + 1] = convexity[i]*(-1);
		row[0] = (double)0;
		set_obj_fn(lp, row);
	}
	// print lp
	print_lp(lp);
	if (solve(lp) == 0) {
		// solve the lp
		cout << "Returned Value from Solve is " << solve(lp) << endl;
		// print optimal value
		cout << "Largest convexity we can get is: " << -1 * get_objective(lp) << endl;

		// print the optimizing values of the variables
		get_variables(lp, solution);
		cout << "Optimal portfolio:" << endl;
		for (int i = 0; i < number_of_cash_flows; i++)
			cout << "%Cash Flow:" << i + 1 << "    " << solution[i] << endl;

		vector<double>lamba;
		vector<int>label;
		for (int i = 0; i < number_of_cash_flows; i++) {
			if (solution[i] != 0) {
				lamba.push_back(solution[i]);
				label.push_back(i + 1);
			}
		}
		cout << "To immunize against small changes in r for each $1 of PV, you should buy:" << endl;
		for (int i = 0; i < lamba.size(); i++)
			cout << "$" << lamba[i] << " of Cash Flow#" << label[i] << endl;
		cout << "If you need to immunize for a larger PV-value, just buy an appropriate propotion" << endl;
		cout << "---------------------------" << endl;

		double a[4] = { 500,750,1000,1009.36 };
		for (int j = 0; j < 4; j++) {
			cout<<"For example, if you want to immunizie for $"<<a[j]<<" of PV, buy:" << endl;
			for (int i = 0; i < lamba.size(); i++)
				cout << "$" << lamba[i]*a[j] << " of Cash Flow#" << label[i] << endl;
			cout << "---------------------------" << endl;
		}

	}
	else {
		cout << "There is no portfolio that meets the duration constraint of 10 years" << endl;
	}

}

int main(int argc, char* argv[])
{
	if (argc == 1) {
		cout << "Input filename missing" << endl;
	}
	else
	{
		get_data(argv);

		print_data(argv[1]);

		get_optimal_portfolio();
	}
	return (0);
}