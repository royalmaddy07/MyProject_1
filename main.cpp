#include<iostream>
#include<climits>
#include<cmath>
#include<vector>
#include<string>
#include<algorithm>
#include<numeric>
#include<fstream>
#include<sstream>
#include<boost/math/distributions/normal.hpp>//the four common distributions in context of stock pricing.
#include<boost/math/distributions/students_t.hpp>
#include<boost/math/distributions/exponential.hpp>
#include<boost/math/distributions/lognormal.hpp>


double empirical_cdf(const std::vector<double>&data, double x){
    return 
    std::count_if(data.begin(),data.end(),[x](double value){return value<=x;})/
    static_cast<double>(data.size());
}//this function calculates and returns eCDF for a data set.

template <typename Dist>
double ks_test_statistic(const std::vector<double>&data, const Dist&dist){
    double max_diff = 0.0;
    for(const auto&x : data){
        double emp_cdf = empirical_cdf(data,x);
        double theo_cdf = boost::math::cdf(dist,x);
        double diff = std::abs(emp_cdf-theo_cdf);
        max_diff = std::max(max_diff,diff);
    }
    return max_diff;
}//function to calculate the test statistic.

void lognormal_parameters(double mean,double stdDev,double &mu,double &sigma){
    double variance = stdDev * stdDev;
    double sigmasquared = std::log(1+(variance/(mean*mean)));
    sigma = std::sqrt(sigmasquared);
    mu = std::log(mean)-(sigmasquared/2.0);
}//function to calculate parameters of the lognormal distributions.

double sample_mean(const std::vector<double>&data){
    return std::accumulate(data.begin(),data.end(),0.0)/data.size();
}//function to calculate the sample mean.

double sample_standard_deviation(const std::vector<double>&data){
    double mean = sample_mean(data);
    double sum_of_squares = 0.0;
    for(double value : data){
        sum_of_squares += std::pow(value - mean,2);
    }
    double variance = sum_of_squares/(data.size()-1);
    return std::sqrt(variance);
}//function to calculate std deviation.


int main() {
    std::ifstream file("historical_data.csv");
    if(!file.is_open()){
        std::cerr<<"FAILED TO OPEN closing_prices.csv FILE";
        return 1;
    }

    std::ofstream output_file("predicted_data.csv");
    if(!output_file.is_open()){
        std::cerr<<"FAILED TO OPEN predicted_values.csv FILE"<<"\n";
        return 1;
    }
    output_file<<"Time,Predicted values\n";

    std::vector<double> data;//vector container to store 20 data points.

    std::string line;
    std::getline(file,line);

    while(std::getline(file,line)){//loop to store the first 20 data points 
        std::stringstream ss(line);
        std::string time, price_str;
        std::getline(ss,time,',');
        std::getline(ss,price_str,',');
        double price = std::stod(price_str);
        data.push_back(price);
        if(data.size()==20){
            break;
        }
    }

    //critical value for alpha = 0.05 and sample size = 20.                            
    double critical_value = 0.29407;

    while(true){
            
        //mean and std deviation calc.
        double mean = sample_mean(data);
        double std_dev = sample_standard_deviation(data);

        //normal distribtion.
        boost::math::normal_distribution<double> normal(mean,std_dev);
        double ks_stat_normal = ks_test_statistic(data,normal);

        //lognormal distribution
        double mu,sigma;
        lognormal_parameters(mean,std_dev,mu,sigma);
        boost::math::lognormal_distribution<double> lognormal(mu,sigma);
        double ks_stat_lognormal = ks_test_statistic(data,lognormal);

        //exponential distribution.
        double lambda = 1.0/mean;
        boost::math::exponential_distribution<double> exponential(lambda);
        double ks_stat_exponential = ks_test_statistic(data,exponential);

        //students t distribtion.
        boost::math::students_t_distribution<double> students(19);
        double ks_stat_students = ks_test_statistic(data,students);

        std::cout<<"normal Qt : "<<ks_stat_normal<<"\n";
        std::cout<<"lognormal Qt : "<<ks_stat_lognormal<<"\n";
        std::cout<<"exponential Qt : "<<ks_stat_exponential<<"\n";
        std::cout<<"t distribution Qt : "<<ks_stat_students<<"\n";

        if(ks_stat_normal>critical_value){
            ks_stat_normal=INT_MAX;
        }
        if(ks_stat_lognormal>critical_value){
            ks_stat_lognormal=INT_MAX;
        }
        if(ks_stat_exponential>critical_value){
            ks_stat_exponential=INT_MAX;
        }
        if(ks_stat_students>critical_value){
            ks_stat_students=INT_MAX;
        }

        double total_weight = 1.0/ks_stat_normal + 1.0/ks_stat_lognormal + 
                            1.0/ks_stat_exponential + 1.0/ks_stat_students;

        double weight_normal = (1.0/ks_stat_normal)/total_weight;
        double weight_lognormal = (1.0/ks_stat_lognormal)/total_weight;
        double weight_exponential = (1.0/ks_stat_exponential)/total_weight;
        double weight_students = (1.0/ks_stat_students)/total_weight;

        double normal_pred = boost::math::quantile(normal,0.5);
        double lognormal_pred = boost::math::quantile(lognormal,0.5);
        double exponential_pred = boost::math::quantile(exponential,0.5);
        double students_pred = boost::math::quantile(students,0.5);

        double pred_value = weight_normal*normal_pred + weight_lognormal*lognormal_pred +
                            weight_exponential*exponential_pred + weight_students*students_pred;
        std::cout<<"the predicted value is : "<<pred_value<<"\n";

        //code to save this generated predicted value.

        if(std::getline(file,line)){
            std::stringstream ss(line);
            std::string time, price_str;
            std::getline(ss,time,',');
            std::getline(ss,price_str,',');
            double new_price = std::stod(price_str);

            output_file<<time<<","<<pred_value<<"\n";

            data.erase(data.begin());
            data.push_back(new_price);
            std::cout<<"UPDATED CLOSING PRICE"<<"\n";
        } else {
            std::cerr<<"ANALYSES OF DATA COMPLETED. EXITING THE PROGRAM....."<<"\n";
            break;
        }                    

    }

    output_file.close();
    file.close();     
    return 0;
}