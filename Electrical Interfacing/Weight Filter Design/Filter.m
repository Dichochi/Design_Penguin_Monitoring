data = [
 3756.25,3756.32,3756.11,3756.77,3756.59,3756.76,3756.78,3757.01,3757.11,3757.11,3757.00,...
3757.19,3756.97,3757.13,3757.47,3757.38,3757.33,3757.46,3757.67,3757.61,3757.53,3757.45,...
3757.50,3757.84,3757.99,3757.48,3757.66,3757.92,3758.11,3758.11,3757.83,3758.02,3757.92,...
3758.15,3758.11,3758.39,3758.41,3758.32,3758.18,3758.18,3758.48,3758.29];

data2=[3272.44,3272.30,3271.84,1500.57,711.16,1432.20,2560.11,2578.67,2578.95,2579.00,...
2575.22,2342.22,1340.89,2474.60,1800.00,3007.04,2992.62,3467.38,3644.72,3645.92,...
3646.65,3647.27,3647.38,3648.37,3648.21,3648.51,3649.04,3649.19,3649.63,3649.79,...
3649.99,3650.50,3650.35,3650.83,3650.85,3650.65,3651.01,3651.02,3651.32,3651.33,...
3651.27,3651.57];

data3=[3565.15,3571.10,3574.31,3577.55,3579.59,3581.29,3583.24,3584.50,3582.43,...
3609.08,1184.48,3121.37,3741.73,3740.27,3741.46,2078.89,983.07,3607.88,3612.60,...
3614.75,3616.61,3618.19,3619.19,3626.09,2639.52,968.43,3832.77,3762.08,3762.48,...
3762.75,3762.72,3762.40,3762.49,3762.40,3762.49,3762.89,3762.49,3762.22,3762.20,...
3762.25,3762.57,3759.98];


%data2=medfilt1(data2,6);
%data3=medfilt1(data3,6);

%data2=movmean(data2,8);
%data3=movmean(data3,8);
alpha = 0.04;
ema = zeros(size(data2));
ema2= zeros(size(data3));
ema(1) = data2(1);  % Initialize the first value
ema2(1)=data3(1);
for k = 2:length(data2)
    ema(k) = (alpha )* data2(k) + (1-alpha) * ema(k - 1);
    ema2(k)=(alpha) * data3(k) + (1-alpha) * ema2(k - 1);
end
data2=ema;
data3=ema2;



% First subplot
subplot(3, 1, 1);
plot(data - 850, 'LineWidth', 1.5, 'Color', 'b');
title('Measured Load at Rest');
xlabel('Sample Index');
ylabel('Weight In grams');
ylim([0 3000]);
xlim([1 42]);
grid on;

% Second subplot
subplot(3, 1, 2);
plot(data2 - 750, 'LineWidth', 1.5, 'Color', 'r');
title('Measured load during aperiodic movement');
xlabel('Sample Index');
ylabel('Weight In grams');
ylim([0 3000]);
xlim([1 42]);
grid on;

% Third subplot
subplot(3, 1, 3);
plot(data3 - 850, 'LineWidth', 1.5, 'Color', 'g');
title('Measured load during periodic movement');
xlabel('Sample Index');
ylabel('Weight In grams');
ylim([0 3000]);
xlim([1 42]);
grid on;