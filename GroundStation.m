% -----------------------------------------------------------------------
%
% Phan mem tram mat dat phuc vu thu tin hieu bong tham khong
% Sinh vien: Pham Duy Thanh
% Truong Dai hoc Cong nghe, Dai hoc Quoc Gia Ha Noi
%
% ------------------------------------------------------------------------
%
clear all;
global DATA;
fp1 = fopen('gs_plot1.log', 'w');

Rearth = 6367*1000; % Ban kinh Trai Dat tinh theo [m]

% Khoi dong;
% ------------
time_old = 0;

% So ve tinh
% ------------
nsat_old = 0;

% Van toc bay
% ------------
speed_old = 0;

% Dien ap pin
% ------------
voltage_old = 0;

% Du lieu GPS
% ---------
X_Target = 0;
Y_Target = 0;
Z_Target = 0;
LONG_TARGET = 10*pi/180; % Vi tri muc tieu duoc tinh theo radian.
LATI_TARGET = 10*pi/180; % Vi tri muc tieu duoc tinh theo radian.
X_old = 0;
Y_old = 0;
Z_old = 0;

clc;
fprintf('Giao tiep giua tram mat dat va thiet bi san sang \n');
A=input('Nhan ENTER de bat dau. Nhan 0 roi nhan ENTER de thoat.\n','s');
if strcmp(A,'0')
    break
end;

scrsz = get(0,'ScreenSize');
H = figure('Position',[50 50 scrsz(3)-100 scrsz(4)-150],'Name','Tram mat dat phuc vu thu tin hieu bong tham khong','NumberTitle','off');

while 1
    COMPORT = serial('COM3','BaudRate', 9600,'Parity', 'none','DataBits',8, 'StopBits',1);
    %COMPORT.Terminator = 'LF';
    %COMPORT.BytesAvailableFcnMode = 'terminator';
    % COMPORT.BytesAvailableFcn = @Serial_Callback;
    %COMPORT.timeout = 100;
    fopen(COMPORT);
    pause(0.6)
    DATA = fscanf(COMPORT);
    fprintf('Du lieu nhan duoc: %s \n',DATA);
    fclose(COMPORT);
    delete(COMPORT);
    
    LDATA= size(DATA);
    VAR_START = strfind(DATA, ',');
    NVAR = size(VAR_START);
    
    % Khai bao bien
    % hh:mm:ss,So ve tinh, Van toc, Vi do, Kinh do, Do cao, Dien ap pin
    
    if NVAR(2) >= 6
        TIME_STR = DATA(1:VAR_START(1)-1);
        NSAT_STR = DATA(VAR_START(1)+1:VAR_START(2)-1);
        SPED_STR = DATA(VAR_START(2)+1:VAR_START(3)-1);
        LATI_STR = DATA(VAR_START(3)+1:VAR_START(4)-1);
        LONG_STR = DATA(VAR_START(4)+1:VAR_START(5)-1);
        ALTI_STR = DATA(VAR_START(5)+1:VAR_START(6)-1);
        VOLT_STR = DATA(VAR_START(6)+1:LDATA(2));
                
        LTIME = size(TIME_STR);
        TIME_START = strfind(TIME_STR,':');
        HOURS_STR = TIME_STR(1:TIME_START(1)-1);
        MINUTES_STR = TIME_STR(TIME_START(1)+1:TIME_START(2)-1);
        SECONDS_STR = TIME_STR(TIME_START(2)+1:LTIME(2));
        time_new = str2double(SECONDS_STR) + 60 * str2double(MINUTES_STR) + 3600 * str2double(HOURS_STR); % Time in Seconds         
            if time_old == 0
                time_old = time_new;
                continue;
            end       
        
        % Ghi du lieu tho
        % So ve tinh
        nsat_new = str2double(NSAT_STR);
        
        % Van toc bay
        speed_new = str2double(SPED_STR);
        
        % Dien ap pin
        voltage_new = str2double(VOLT_STR);
        
        LATI = str2double(LATI_STR)*pi/(180*60*60);
        LONG = str2double(LONG_STR)*pi/(180*60*60);
        
        X_new = Rearth*(LONG - LONG_TARGET);% CALCULATION IS NEEDED
        Y_new = Rearth*(LATI - LATI_TARGET);% CALCULATION IS NEEDED
        Z_new = str2double(ALTI_STR);% CALCULATION IS NEEDED       
    else
        time_new = time_old;
        nsat_new = nsat_old;
        speed_new = speed_old;
        voltage_new = voltage_old;
        
        X_new = X_old;
        Y_new = Y_old;
        Z_new = Z_old;
    end

%
% Ve do thi
% -------------
%
% Du lieu GPS
% --------
subplot(3,3,[2 3 5 6 8 9]);
plot3([X_old X_new], [Y_old Y_new], [Z_old Z_new],'r-');
grid ON
title('Quy dao bong tham khong');
hold on;
plot3(X_Target, Y_Target, Z_Target,'ro','MarkerSize',20,'MarkerFaceColor','r');
hold on;

% So ve tinh
% --------------
subplot(3,3,1);
plot([time_old time_new], [nsat_old nsat_new],'b--');
title('So ve tinh');
hold on;

% Van toc bay
% --------------
subplot(3,3,4);
plot([time_old time_new], [speed_old speed_new],'b--');
title('Van toc bay');
hold on;

% Dien ap pin
% --------------
subplot(3,3,7);
plot([time_old time_new], [voltage_old voltage_new],'b--');
title('Dien ap pin');
hold on;
pause(0.2);

%
% CAP NHAT
% ------
%
% So ve tinh
% ---------------------------
nsat_old = nsat_new;

% Van toc bay
% ---------------------------
speed_old = speed_new;

%  Dien ap pin
% ---------------------------
voltage_old = voltage_new;

% Du lieu GPS
% ---------
X_old = X_new;
Y_old = Y_new;
Z_old = Z_new;

% Thoi gian
% ----
time_old = time_new;

%
% LUU DU LIEU VAO FILE
% ------------------
fprintf(fp1,'%f, %f , %f, %f, %f, %f, %f\n',time_new, nsat_new, speed_new, X_new, Y_new, Z_new, voltage_new);
%
% Dong cong Serial
%
end
fclose(fp1);