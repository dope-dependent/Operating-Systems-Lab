/*
    Operating Systems Laboratory (CS39002)
    
    Group - 26

    Seemant G. Achari  (19CS10055)
    Rajas Bhatt  (19CS30037)

    Assignment 2

    File  - Assignment2_26_19CS30037_19CS10055.cpp
*/

/* C libraries */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
/* C++ libraries*/
#include <string> 
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <deque>
#include <map>
#include <chrono>

using namespace std;

/* inotify headers (predefined in some systems) */
#define ICANON 0000002
#define ECHO 0000010
#define TCSANOW 0
/* Maximum history size (to be stored in the ./shell_history file) */
#define MAX_HISTORY_SIZE 10000

/* Terminal Mode switchers */
bool canonMode = true;
void switchTerminalMode() {
    if (canonMode) {
        struct termios old_tio, new_tio;
        /* get the terminal settings for stdin */
        tcgetattr(STDIN_FILENO, &old_tio);
        /* we want to keep the old setting to restore them a the end */
        new_tio = old_tio;
        /* disable canonical mode (buffered i/o) and local echo */
        new_tio.c_lflag &= ~(ICANON | ECHO);
        /* set the new settings immediately */
        tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);
        canonMode = false;
    }
    else {
        struct termios old_tio, new_tio;
        /* get the terminal settings for stdin */
        tcgetattr(STDIN_FILENO, &old_tio);
        /* we want to keep the old setting to restore them a the end */
        new_tio = old_tio;
        /* disable canonical mode (buffered i/o) and local echo */
        new_tio.c_lflag |= (ICANON | ECHO);
        /* set the new settings immediately */
        tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);
        canonMode = true;
    }   
}


class History;
class Line;
class Command;
class Terminal;

void execute_command(const Command &);
void execute_pipe(vector<Command> &);
void execute_pipe2(vector <Command> &v);

/* To maintain shell history */
class History{
    string _history_file;       // File in which history is stored
    int _max_history_size;      // Maximum size of history
    deque <string> _history;    // The actual history
public:
    History() {
        _history_file = "./.shell_history";
        _max_history_size = MAX_HISTORY_SIZE;
        int fd = open(_history_file.c_str(), O_CREAT | O_RDONLY, 0666);
        if (fd < 0) {
            perror("shell : ");
            exit(EXIT_FAILURE);
        }   
        char buf[1001];
        int rx;
        _history.push_back({});
        while (rx = read(fd, buf, 1000)) {
            if (rx < 0) {
                perror("shell : ");
                exit(EXIT_FAILURE);
            }
            for (int i = 0; i < rx; i++) {
                if (buf[i] == '\n') _history.push_back({});
                else _history.back().push_back(buf[i]);
            }
        }
        if (_history.size()) _history.pop_back();
    }
    ~History() {
        int fd = open(_history_file.c_str(), O_WRONLY | O_CREAT, 0666);
        if (fd < 0) {
            perror("shell : ");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < _history.size(); i++) {
            _history[i] += '\n';
            char *buf = (char *)calloc(_history[i].length() + 1, sizeof(char));
            strcpy(buf, _history[i].c_str());
            if (write(fd, buf, _history[i].length()) < 0) {
                perror("shell : ");
                exit(EXIT_FAILURE);
            }
            // TODO
            free (buf);
        }
    }
    void printHistory() {
        for (int i = 0; i < min(_history.size(), 1000ul); i++) {
            cout << "\t" << _history[_history.size() - i - 1] << "\n";
        }
        cout << endl;
    }
    void searchHistory() {
        cout << endl << "shell_search: ";
        string term;

        // Change terminal to canon mode
        switchTerminalMode();
        // Get user input
        getline(cin, term);
        // Change terminal to non-canon mode
        switchTerminalMode();

        vector <pair<int, int>> counts; // Matching counts
        bool no_match = false;  // No match found
        // Iterate over most recent to least recent history
        for (int i = _history.size() - 1; i >= 0; i--) {
            int M = _history[i].length(), N = term.length(), ans = 0;
            // Use dp to find the longest common substring length
            vector <vector<int>> dp (M + 1, vector<int> (N + 1, 0));
            for (int m = 0; m <= M; m++) {
                for (int n = 0; n <= N; n++) {
                    if (m == 0 || n == 0) continue;
                    if (_history[i][m - 1] == term[n - 1]) {
                        dp[m][n] = 1 + dp[m - 1][n - 1];
                        ans = max(ans, dp[m][n]);
                    }
                }
            }
            // Complete match found
            if (ans == M && ans == N) {
                cout << "\n\tComplete match found : " << _history[i] << "\n" << endl;
                return;
            }
            // Will store the match length of each index
            counts.push_back({ans, i});
        }
        sort(counts.rbegin(), counts.rend(), greater<pair<int, int>>());
        reverse(counts.begin(), counts.end());
        
        if (counts.size() == 0) no_match = true;
        else {
            if (counts.front().first <= 2) no_match = true;
            else {
                cout << "\nMaximum length matches (from most recent to least recent) : \n" << endl;
                for (int i = 0; i < counts.size(); i++) {
                    if (counts[i].first != counts.front().first) break;
                    cout << "\t" << counts[i].second << " " << _history[counts[i].second] << "\n"; 
                }
                cout << endl;
            }            
        }
        if (no_match) {
            cout << "\n\tNo match for search term in history\n" << endl;
        }
    }
    void addToHistory(const string &s) {
        if (_history.size() >= MAX_HISTORY_SIZE) {
            _history.pop_front();
            _history.push_back(s);
        }
        else _history.push_back(s);
    }
};

/* Global shell history */
History shell_hist;

/* Handles Basic Terminal Functionality and Autocomplete */
class Terminal{
    string _ip;

public:
    Terminal() {
        int i = 0;
        while(true){
            char c = getchar(); 
            if(c == '\t') Complete(i); // Trigger autocomplete          
            else if(c == 18) {         // Ctrl + R
                shell_hist.searchHistory();
                _ip.clear();
                break;
            }           
            else if(c == 127) {  // Backspace
                if(_ip.length()>0){
                    _ip.pop_back();
                    fputs("\b \b",stdout);
                }   
            } 
            else if (c == '\n') {   // Newline
                putchar(c);
                break;
            }           
            else if (c >= 32 && c <= 127) { // Printables
                putchar(c);
                _ip.push_back(c);
            }
        }
    } 

    void Complete(int index) {
        struct dirent *d;
        DIR *dr;
        dr = opendir("./.");
        if(dr!=NULL)
        {
            vector<string> lis;

            string lastFile;

            for(int i = _ip.length() - 1; i >=0; i--)
                if(_ip[i]==' ') break;
                else lastFile.push_back(_ip[i]);
            
            reverse(lastFile.begin(),lastFile.end());
            
            for(d=readdir(dr); d!=NULL; d=readdir(dr))
                if(lastFile.length()<=strlen(d->d_name))
                lis.push_back(string(d->d_name));
            
            vector<string> match=getList(lastFile,lis);
            string toFill;
            if(match.size()==1) toFill=match[0];
            else if (match.size()>1){
                string subtring=getSubstring(match);
                if(subtring.length()!=0)
                
                if(subtring.length()>lastFile.length())
                toFill=subtring;
                
                else{
                    printf("\nPlease choose from among the following choices:\n");
                    for(int i=0;i<match.size();i++)
                    cout<<i+1<<". "<<match[i]<<endl;
                    printf("\n\nEnter your choice: ");
                    int choice; string number;
                    while(1){
                        char c = getchar();                     
                        if(c==127) {
                            if(number.length()>0){
                                number.pop_back();
                                fputs("\b \b",stdout);
                            }   
                        }
                        
                        else {
                            putchar(c);
                            if(c=='\n') break;
                            number.push_back(c);
                        }
                    }  
                    choice=atoi(number.c_str());
                    toFill=match[choice-1];
                    cout<<endl<<"~ "<<_ip;
                }
            }
            Fill(toFill);
            closedir(dr);
        }
        else {
            // ERROR
        }
    }
    
    void Fill(const string &fillWith) {
        string lastFile;

        for(int i=_ip.length()-1;i>=0;i--)
        if(_ip[i]==' ') break;
        else lastFile.push_back(_ip[i]);

        for(int i=lastFile.size();i<fillWith.size();i++)
        {
            fputc(fillWith[i],stdout);
            _ip.push_back(fillWith[i]);
        }
    }
    
    string getLine() {return _ip;}

    string getSubstring(vector<string>& match){
        string x;
        int len=100000;
        for(int i=0;i<match.size();i++)
        len=min(len,match[i].length());

        for(int i=0;i<len;i++){
            int j;
            for(j=1;j<match.size();j++)
            if(match[j-1][i]!=match[j][i]) break;
            if(j==match.size()) x.push_back(match[0][i]);
            else break;
        }
        return x;
    }

    int comp(string cmd, string str){
        for(int i=0;i<cmd.length();i++)
        if(cmd[i]>str[i]) return 1;
        else if(cmd[i]<str[i]) return -1;
        return 0;
    }

    int getLower(string cmd,vector<string>& lis){
        int lower=-1, higher=lis.size();
        int mid=(lower+higher)/2;
        while(higher-lower>1){
            if(comp(cmd,lis[mid])>0) lower=mid;
            else higher=mid;
            mid=(lower+higher)/2;
        }
        return lower;
    }

    int getHigher(string cmd,vector<string>& lis){
        int lower=-1, higher=lis.size();
        int mid=(lower+higher)/2;
        while(higher-lower>1){
            if(comp(cmd,lis[mid])>=0) lower=mid;
            else higher=mid;
            mid=(lower+higher)/2;
        }
        return higher;
    }
    
    int min(int a, int b){
        if(a>b) return b;
        return a;
    }
    
    vector<string> getList(string lastFile,vector<string>& lis){
        vector<string> ans;
        sort(lis.begin(), lis.end());

        int lower=getLower(lastFile, lis);
        int higher=getHigher(lastFile, lis);


        for(int i=lower+1;i<higher;i++)
        ans.push_back(lis[i]);

        return ans;
    }    
};


void ctrlchandler(int signum) {
    // signal(SIGINT, ctrlchandler);
    // cout << "\nAbort" << getpid() << "\n";
    fflush(stdout);
}
void ctrlzhandler(int signum) {
    // cout << "Send to background "<< getpid() << endl;
    fflush(stdout);
}

void endhandler(int signum) {
    fflush(stdout);
}



/* Controls tokenization */
class Line{
private:
    string _line;
    vector<string> _pipe_tokens;
    vector<string> _space_tokens;
    int _error;
public:
    Line() {}
    Line(const string &_ip) {
        _line = _ip;
        remSpace(_line);
        if (_line.length() == 0) _error = 1;
        else _error = 0;
        pipeTokenize();
        spaceTokenize();
    } 
    void pipeTokenize() {   // Tokenize the line with pipe separators
        stringstream l(_line);
        string temp;
        while (getline(l, temp, '|')) {
            if (temp.length()) _pipe_tokens.push_back(temp);
        }  
    }
    void spaceTokenize() {  // Tokenize the line with space separators
        stringstream l(_line);
        string temp;
        while (getline(l, temp, ' ')) {
            if (temp.length()) _space_tokens.push_back(temp);
        } 
    }
    static void remSpace(string &c){    // Remove spaces from the beginning and end
        int i,j;
        for(i=0;i<c.length();i++) if(c[i]!=' ') break;
        for(j=c.length()-1;j>=0;j--) if(c[j]!=' ') break;
        c=c.substr(i,j-i+1);
    }
    vector <string> getPipeTokens() {return _pipe_tokens;}
    vector <string> getSpaceTokens() {return _space_tokens;}
    int getError() {return _error;}
    string getInput() {return this->_line;}
};

/* Details about a single command */
class Command{
public:
    string _cmdentry;
    vector <string> _cmd_tok;
    string _cmdname;
    int _n_args;
    char **_args;
    bool _back;
    string _ip_file;
    string _op_file;
    int _ip_desc;
    int _op_desc;

    Command() {}
    Command(string s) {
        _args = nullptr;
        _cmdentry = s;     
        _back = false; 
        _ip_desc = 0;
        _op_desc = 1;  
        Line l(_cmdentry);
        _cmd_tok = l.getSpaceTokens();
        parse();
        _cmdname = _cmd_tok[0];
        _n_args = _cmd_tok.size();
        _args = (char **)malloc((_n_args + 1) * sizeof(char *));
        for (int i = 0; i < _n_args; i++) {
            _args[i] = strdup(_cmd_tok[i].c_str());
        }
       
        _args[_n_args] = nullptr;
    }
    Command(const Command &c) {
        _cmdentry = c._cmdentry;
        _cmd_tok = c._cmd_tok;
        _cmdname = c._cmdname;
        _n_args = c._n_args;
        _back = c._back;
        _ip_file = c._ip_file;
        _op_file = c._op_file;
        _args = (char **)malloc((_n_args + 1) * sizeof(char *));
        for (int i = 0; i < _n_args; i++) {
            _args[i] = strdup(c._args[i]);
        }
        _args[_n_args] = nullptr;
    }
    void add_desc (int ip_desc = 0, int op_desc = 1) {
        _ip_desc = ip_desc;
        _op_desc = op_desc;
    }
    void parse() {
        // TODO
        // 1. Check for proper ip and op file by user
        // 2. Reduce the length of this code maybe?
        for (int i = 0; i < _cmd_tok.size(); ) {
            string arg = _cmd_tok[i];
            if (arg.front() == '>') {
                if (arg.length() > 2) {
                    _op_file = arg.substr(1, arg.length() - 1);
                    _cmd_tok.erase(_cmd_tok.begin() + i);
                }
                else {
                    _cmd_tok.erase(_cmd_tok.begin() + i);
                    if (i != _cmd_tok.size()) {
                        _op_file = _cmd_tok[i];
                        _cmd_tok.erase(_cmd_tok.begin() + i);
                    }
                }
            }
            else if (arg.front() == '<') {
                if (arg.length() > 2) {
                    _ip_file = arg.substr(1, arg.length() - 1);
                    _cmd_tok.erase(_cmd_tok.begin() + i);
                }
                else {
                    _cmd_tok.erase(_cmd_tok.begin() + i);
                    if (i != _cmd_tok.size()) {
                        _ip_file = _cmd_tok[i];
                        _cmd_tok.erase(_cmd_tok.begin() + i);
                    }
                }
            }
            else i++;            
        }      
        // Background process check using &
        if (_cmd_tok.back() == "&") {
            _back = true;
            _cmd_tok.pop_back();
        }
    }
    ~Command() {
        for (int i = 0; i < _n_args + 1; i++) {
            free (_args[i]);
        }
        free (_args);
    }  
};


class MultiWatch{
    string _ip;
    int _n_cmds;
    vector <vector<Command>> _cmds;
    vector <string> _pipes;
    int _p_pid;
    vector <int> _pids;
    map <string, pair<int, int>> _data;
    static int _running;
    static int _pid_child;
    static map<int, int> _pid_fd;
    static int _inotify_fd;

public:
    MultiWatch() {}
    MultiWatch(const string &s) {
        _ip = s;
        _n_cmds = 0;
        _p_pid = 0;
        this->parse();
    }
    static void chld_handler(int sign) {
        _running--;
        // cout << _running << endl;
        if (_running <= 0) close(_inotify_fd);
    }
    static void end_handler(int sign) {
        int st;
        int proc = wait3(&st, WNOHANG, NULL);
        close(_pid_fd[proc]);
    }
    static void control_handler(int sign) {
        _running = 0;
        if (_running <= 0) close(_inotify_fd);
    }
    void parse() {
        int _lsb = _ip.find('[');
        int _msb = _ip.find(']');
        // cout << _lsb << " " << _msb << endl;
        // TODO
        if (_msb == -1 || _lsb == -1) {
            perror("sh : Wrong syntax");
            exit(EXIT_FAILURE);
        }
        string base = _ip.substr(_lsb + 1, _msb - _lsb - 1);
        // cout << base << endl;
        // TODO
        if (base.length() == 0) return;
        stringstream st(base);
        string temp;
        vector <string> temp_c;
        while (getline(st, temp, ',')) {
            if (temp.length() > 2) {
                Line::remSpace(temp);
                string cmd=temp.substr(1, temp.length() - 2);
                Line::remSpace(cmd);
                temp_c.push_back(cmd);                
            }
        }
        for (auto pipe : temp_c) {
            Line l(pipe);
            vector <string> s = l.getPipeTokens();
            vector <Command> cmds;
            if (s.size()) {
                _pipes.push_back(pipe);
                for (auto cmd : s) {
                    cmds.push_back(Command(cmd));
                }
                _cmds.push_back(cmds);
                _n_cmds++;                
            }            
        }
        _running = _n_cmds;
        // cout << _n_cmds << endl;
    }
    void execute() {
        _p_pid = getpid();
        _pid_fd.empty();
        // cout << _p_pid << endl;
        _pids.resize(_n_cmds);
        for (int i = 0; i < _n_cmds; i++) {
            int x = fork();
            // cout << x << endl;
            if (x == 0) {
                string cf = ".temp.";
                cf += to_string(getpid());
                cf += ".txt";
                int op = open(cf.c_str(), O_CREAT|O_RDWR, 0666);
                _cmds[i].back()._op_file = cf;
                // execute_pipe(_cmds[i]);
                execute_pipe2(_cmds[i]);
                kill(getppid(), SIGUSR1);
                exit(EXIT_SUCCESS);
            }
            else {
                _pids[i] = x;
                string cf = ".temp.";
                cf += to_string(x);
                cf += ".txt";
                int fd = open(cf.c_str(), O_CREAT|O_RDONLY, 0666);
                _pid_fd.insert({x, fd});
                _data.insert({cf.c_str(), {i, fd}});
                continue;
            }
        }
        // cout << getpid() << endl;
        // cout << "Got parent " << getpid() << endl;
        _inotify_fd = inotify_init();
        if (_inotify_fd < 0) {
            perror("sh ");
            exit(EXIT_FAILURE);
        }
        map <int, string> wds;
        for (auto element : _data) {
            // cout << element.first << endl;
            int wd = inotify_add_watch(_inotify_fd, element.first.c_str(), IN_MODIFY);
            if (wd < 0) {
                perror("sh ");
                exit(EXIT_FAILURE);
            }
            wds.insert({wd, element.first.c_str()});
        }
        char buf[1000];
        char buf2[20000];
        while (_running > 0)
        {
            if (_running <= 0) break;
            // cout<<"running "<<_running<<endl;
            // cout << flag2 << endl;
            signal(SIGINT, control_handler);
            signal(SIGUSR1, chld_handler);
            signal(SIGCHLD, end_handler);
            int r = read(_inotify_fd, buf, 1000);
            if (r <= 0) break;
            else {
                // cout << r << "\n";
                struct inotify_event *event = (struct inotify_event *)buf;
                string en(wds[event->wd]);
                int rx;
                pair <int, int> ddata = _data[en];
                int index = ddata.first;
                int fd = ddata.second;
                int wd = event->wd;
                // cout << wd << endl;
                if (_running <= 0) break;
                while (_running > 0) {
                    rx = read(fd, buf2, 20000);
                    if (rx <= 0) break;
                    time_t rt;
                    time(&rt);
                    struct tm *info;
                    info = gmtime(&rt);
                    cout << _pipes[index] << ", " << info->tm_hour << ":" << info->tm_min << ":" << info->tm_sec << endl;
                    // cout << rx << endl;
                    cout << "<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-" << endl;
                    for (int i = 0; i < rx; i++) {
                        cout << buf2[i];
                    }
                    cout << "->->->->->->->->->->->->->->->->" << endl;
                }
                cout << endl;
            }
        }   
        // cout << "F" << endl;
        for (auto element : _data) {
            remove(element.first.c_str());
        }
    }
};

int MultiWatch::_running = 0;
int MultiWatch::_inotify_fd = 0;
map <int, int> MultiWatch::_pid_fd;

void handlerparent(int num){
    cout << "\nsh : Execution moved to background" << endl;
}

void handlerparent_stop(int num){
    kill(getpid(),SIGSTOP);
}

/* Executes a single non-piped command */
void execute_command(const Command &c) {
    int fd_in = c._ip_desc, fd_out = c._op_desc;
    switchTerminalMode();
    int x = fork();
    if (x < 0) {
        perror("sh : fork error");
        exit(EXIT_FAILURE);
    }
    if (x == 0) {
        if (c._ip_file != "") {
            int fd_in = open(c._ip_file.c_str(), O_RDONLY); 
            if (fd_in < 0) perror("sh ");
            else dup2(fd_in, 0);
        }
        if (c._op_file != "") {
            int fd_out = open(c._op_file.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0666);
            if (fd_out < 0) perror("sh ");
            else dup2(fd_out, 1);
        }
        int op = execvp(c._cmdname.c_str(), c._args);
        if (op < 0 && !c._back) {
            perror("sh ");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    else {
        signal(SIGTSTP, handlerparent);  
        int st; 
        if (!c._back) {
            int err = waitpid(x, &st, WUNTRACED);
            if (err < 0) {
                perror("sh ");
                exit(EXIT_FAILURE);
            }
            if (WIFSTOPPED(st)) {
                // cout << "Plan to restart the process!" << endl;
                kill(x, SIGCONT);
            }
        }      
    }
    switchTerminalMode();
}

/* Executes a piped command */
void execute_pipe(vector <Command> &v) {
    // TODO CHECK IF SOMETHING ENTERED v.size() == 0 CASE
    if (v.size() == 0) return;
    if (v.size() == 1) {
        execute_command(v[0]);
        return;
    }

    vector <pair<int, int>> pipes;
    
    for (int i = 0; i < v.size() - 1; i++) {
        int pipefd[2];
        if (pipe(pipefd)) {
            perror("sh : pipe error");
            exit(EXIT_FAILURE);
        }
        pipes.push_back({pipefd[0], pipefd[1]});
    }
    // TODO CHECK dup2() errors in if and else statements
    int x = fork();
    if (x < 0) {
        perror("sh : fork error");
        exit(EXIT_FAILURE);
    }
    if (x == 0) {      
        // FIRST PROCESS in the pipe
        // TODO ERROR HANDLE OUTPUT
        // TODO CHECK INPUT FROM FILE IN FIRST PIPE COMMAND (CMD < 1.TXT | CMD2)
        if (v[0]._ip_file != "") {
            int fd_in = open(v[0]._ip_file.c_str(), O_RDONLY); 
            if (fd_in < 0) {
                perror("sh ");exit(EXIT_FAILURE);
            }
            else {
                int err = dup2(fd_in, 0);
                if (err < 0) {
                    perror("sh : duplication error");exit(EXIT_FAILURE);
                }
            }
        }
        if (v[0]._op_file != "") {
            cout << "sh : Warning! Redirection in between pipes will do nothing" << endl;
        }
        int err = dup2(pipes[0].second, 1);
        if (err < 0) {
            perror("sh : duplication error");exit(EXIT_FAILURE);
        }
        err = execvp(v[0]._cmdname.c_str(), v[0]._args);
        if (err < 0) {
            perror("sh ");exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    else {
        // wait(NULL);
        signal(SIGTSTP, handlerparent);  
        int st;
        if (waitpid(x, &st, WUNTRACED) < 0) {
            perror("sh ");
            exit(EXIT_FAILURE);
        }
        if (WIFSTOPPED(st)) {
            cout << "Plan to restart the process!" << endl;
            kill(x, SIGCONT);
        }
        if (close(pipes[0].second) < 0) {
            perror("sh ");
            exit(EXIT_FAILURE);
        }
        for (int i = 1; i < v.size(); i++) {
            x = fork();
            if (x < 0) {
                perror("sh : fork error");
                exit(EXIT_FAILURE);
            }
            if (x == 0) {                    
                if (dup2(pipes[i - 1].first, 0) < 0) {
                    perror("sh : duplication error");
                    exit(EXIT_FAILURE);
                }
                // INTERMEDIATE PROCESS
                // TODO ERROR HANDLE BOTH INPUT AND OUTPUT FOR MIDDLE PROCESSES
                if (i != v.size() - 1) {
                    if (dup2(pipes[i].second, 1) < 0) {
                        perror("sh : duplication error");
                        exit(EXIT_FAILURE);
                    }
                }
                else {
                    // OUTPUT PROCESS
                    // TODO ERROR HANDLE IF INPUT GIVEN
                    if (v[i]._op_file != "") {
                        int fd_out = open(v[i]._op_file.c_str(), O_WRONLY|O_CREAT, 0666); 
                        if (fd_out < 0) perror("ERROR ");
                        else dup2(fd_out, 1);
                    }
                }
                execvp(v[i]._cmdname.c_str(), v[i]._args);
            }
            else {
                // Parent process which will create more children
                // wait(NULL);
                // signal(SIGTSTP, handlerparent_stop);  
                int st; 
                // kill(x, SIGTSTP);
                waitpid(0, &st, WUNTRACED);
                if (WIFSTOPPED(st)) {
                    cout << "Plan to restart the process!" << endl;
                    kill(x, SIGCONT);
                }
                close(pipes[i - 1].first);
                if (i != v.size() - 1) close(pipes[i].second);
            }
        }
    }
    
}

void execute_pipe2(vector <Command> &v) {
    // TODO CHECK IF SOMETHING ENTERED v.size() == 0 CASE
    if (v.size() == 0) return;
    if (v.size() == 1) {
        execute_command(v[0]);
        return;
    }

    int x=fork();
    if(x==0){
        vector <pair<int, int>> pipes;
        
        for (int i = 0; i < v.size() - 1; i++) {
            int pipefd[2];
            if (pipe(pipefd) < 0) {
                perror("sh : pipe error");
                exit(EXIT_FAILURE);
            }
            pipes.push_back({pipefd[0], pipefd[1]});
        }
        // TODO CHECK dup2() errors in if and else statements
        for (int i = 0; i < v.size(); i++) {
            int x = fork();
            if (x < 0) {
                perror("sh : fork error");
                exit(EXIT_FAILURE);
            }
            if (x == 0) {
                if (i == 0 && v[i]._ip_file != "") {
                    int fd_in = open(v[i]._ip_file.c_str(), O_RDONLY); 
                    if (fd_in < 0) {
                        perror("sh ");exit(EXIT_FAILURE);
                    }
                    else {
                        int err = dup2(fd_in, 0);
                        if (err < 0) {
                            perror("sh : duplication error");exit(EXIT_FAILURE);
                        }
                    }
                }
                if (i == v.size() - 1 && v[i]._op_file != "") {
                    int fd_out = open(v[i]._op_file.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0666); 
                    if (fd_out < 0) {
                        perror("sh ");
                        exit(EXIT_FAILURE);
                    }
                    else {
                        if (dup2(fd_out, 1) < 0) {
                            perror("sh : duplication error");
                            exit(EXIT_FAILURE);
                        };
                    }
                }
                if (i != 0 && v[i]._ip_file != "") {
                    cout << "sh : Warning! Redirection in between pipes will do nothing" << endl;
                }
                if (i != v.size() - 1 && v[i]._op_file != "") {
                    cout << "sh : Warning! Redirection in between pipes will do nothing" << endl;
                }
                if (i != 0 && dup2(pipes[i - 1].first, 0) < 0) {
                    perror("sh : duplication error");
                    exit(EXIT_FAILURE);
                }
                if (i != v.size() - 1 && dup2(pipes[i].second, 1) < 0) {
                    perror("sh : duplication error");
                    exit(EXIT_FAILURE);
                } 
                int err = execvp(v[i]._cmdname.c_str(), v[i]._args);
                if (err < 0) {
                    perror("sh ");
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);
            }
            else {
                signal(SIGTSTP, handlerparent_stop);  
                int st;
                if (waitpid(x, &st, WUNTRACED) < 0) {
                    perror("sh ");
                    exit(EXIT_FAILURE);
                }
                if (WIFSTOPPED(st)) {
                    cout << "Plan to restart the process!" << endl;
                    kill(x, SIGCONT);
                    wait(NULL);
                }
                if (i != 0 && close(pipes[i - 1].first) < 0) {
                    perror("sh ");
                    exit(EXIT_FAILURE);
                }
                if (i != v.size() - 1 && close(pipes[i].second) < 0) {
                    perror("sh ");
                    exit(EXIT_FAILURE);
                }
                // cout<<"Process "<<i<<" completed\n";
            }
        }
        exit(EXIT_SUCCESS);
    }

    else{
        //wait until above child finishes by default
        int st;
        if(!v.back()._back){
            if (waitpid(x, &st, WUNTRACED) < 0) {
                    perror("sh ");
                    exit(EXIT_FAILURE);
                }
            if (WIFSTOPPED(st)) {
                cout << "Plan to restart the executor process!" << endl;
                kill(x, SIGCONT);
                
            }
        }
        
    }

    
}

int parent_pid;

int main() {  
    switchTerminalMode();

    while (true) {
        signal(SIGINT, ctrlchandler);
        signal(SIGTSTP, ctrlzhandler);
        signal(SIGCHLD, endhandler);
        parent_pid = getpid();
        // cout << parent_pid << endl;
        cout << "~ ";  
        Terminal t;
        Line l(t.getLine());
        // ADDED
        if (l.getError()) continue;
        vector <string> pipes = l.getPipeTokens();
        vector <string> strs = l.getSpaceTokens();
        vector <Command> cmds;
        // ADDED
        if (strs[0] == "history") shell_hist.printHistory();
        else if (strs[0] == "exit") break;
        else if (strs[0] == "multiWatch") {
            MultiWatch m(t.getLine());
            m.execute();
        }
        else {
            for (int i = 0; i < pipes.size(); i++) {
                cmds.emplace_back(Command(pipes[i]));
            }
            // execute_pipe(cmds);
            execute_pipe2(cmds);  
                      
        }      
        // ADDED
        cout << endl;
        shell_hist.addToHistory(l.getInput());      
    }
    
    switchTerminalMode();
    return 0;
}