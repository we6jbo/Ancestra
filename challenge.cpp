#include "challenge.h"
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
static constexpr const char* scriptPath = "/usr/local/bin/challenge.sh";
static constexpr const char* keyPath = "/home/we6jbo/.ads/challenge.key";
static constexpr const char* timePath = "/tmp/fwd/challenge_time.txt";
static std::string hex(const unsigned char* p,size_t n){static constexpr char d[]="0123456789abcdef";std::string r;r.reserve(n*2);for(size_t i=0;i<n;++i){r+=d[p[i]>>4];r+=d[p[i]&15];}return r;}
static std::vector<unsigned char> unhex(const std::string& v){if(v.size()!=64)throw std::runtime_error("Invalid 256-bit key length");std::vector<unsigned char> out;for(size_t i=0;i<v.size();i+=2){auto digit=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;return -1;};int a=digit(v[i]),b=digit(v[i+1]);if(a<0||b<0)throw std::runtime_error("Invalid key encoding");out.push_back(static_cast<unsigned char>((a<<4)|b));}return out;}
static std::string readOneLine(const char* filename){std::ifstream in(filename);if(!in)throw std::runtime_error(std::string("Cannot open ")+filename);std::string line;if(!std::getline(in,line))throw std::runtime_error("Empty file");return line;}
static std::string roundUpLocalTime(){std::time_t now=std::time(nullptr);now=((now+299)/300)*300;std::tm result{};if(!localtime_r(&now,&result))throw std::runtime_error("localtime_r failed");char s[64]{};if(!std::strftime(s,sizeof(s),"%Y-%m-%dT%H:%M:%S%z",&result))throw std::runtime_error("strftime failed");return s;}
static std::string expectedResponse(const std::string& nonce,const std::string& stamp){auto key=unhex(readOneLine(keyPath));const std::string message="challenge-v1|"+nonce+"|"+stamp;unsigned char digest[EVP_MAX_MD_SIZE]{};unsigned int count=0;if(!HMAC(EVP_sha256(),key.data(),static_cast<int>(key.size()),reinterpret_cast<const unsigned char*>(message.data()),message.size(),digest,&count))throw std::runtime_error("HMAC failed");return hex(digest,count);}
struct ScriptResult{int exitCode;std::string output;};
static ScriptResult runScript(const std::string& nonce){int fds[2];if(pipe(fds)!=0)throw std::runtime_error("pipe failed");pid_t pid=fork();if(pid<0){close(fds[0]);close(fds[1]);throw std::runtime_error("fork failed");}if(pid==0){close(fds[0]);dup2(fds[1],STDOUT_FILENO);close(fds[1]);execl(scriptPath,scriptPath,nonce.c_str(),(char*)nullptr);_exit(127);}close(fds[1]);std::string output;char buffer[256];ssize_t n;while((n=read(fds[0],buffer,sizeof buffer))>0)output.append(buffer,n);close(fds[0]);int status=0;if(waitpid(pid,&status,0)<0)throw std::runtime_error("waitpid failed");return {WIFEXITED(status)?WEXITSTATUS(status):-1,output};}
std::string performChallenge(){for(int attempt=0;attempt<10;++attempt){std::array<unsigned char,16> bytes{};if(RAND_bytes(bytes.data(),bytes.size())!=1)throw std::runtime_error("RAND_bytes failed");const std::string nonce=hex(bytes.data(),bytes.size());const auto result=runScript(nonce);if(result.exitCode==3&&result.output=="DUPLICATE\n")continue;if(result.exitCode!=0)throw std::runtime_error("Script failed: "+result.output);std::istringstream in(result.output);std::string status,stamp,response,extra;if(!(in>>status>>stamp>>response)||(in>>extra)||status!="OK")throw std::runtime_error("Invalid script response");if(response!=expectedResponse(nonce,stamp))throw std::runtime_error("Challenge response authentication failed");if(readOneLine(timePath)!=stamp||roundUpLocalTime()!=stamp)throw std::runtime_error("time sync does not match");return stamp;}throw std::runtime_error("Too many duplicate random challenges");}
bool verify(){try{std::cout<<"Verified time: "<<performChallenge()<<'\n';return true;}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return false;}}
