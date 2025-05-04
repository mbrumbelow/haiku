/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alex von Gluck, <alex@terarocket.io>
 */

#include <curl/curl.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

struct RepoMirror {
	std::string country;
	std::string city;
	std::string provider;
	std::string website;
	CURLU *url;
	double metric;
};

uint16_t
checksum(void *b, int len)
{
	unsigned short *buf = (unsigned short*)b;
	uint32_t sum=0;
	while (len > 1) {
		 sum += *buf++;
		 len -= 2;
	}
	if(len == 1)
		 sum += *(unsigned char*)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	return ~sum;
}

double
icmp_ping(const char *hostname, int count, double timeout)
{
	struct sockaddr_in dest_addr;
	struct timespec time_start, time_end, tfs, tfe;
	double rtt = 0.0;
	int sent = 0, received = 0;

	// XXX: This requires root on linux. Good thing Haiku is uid 0 ;-)
	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) {
		perror("Failed to create raw socket");
		return -1.0;
	}

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	inet_pton(AF_INET, hostname, &dest_addr.sin_addr);

	clock_gettime(CLOCK_MONOTONIC, &tfs);
	for (int i = 0; i < count; ++i) {
		struct icmphdr icmp_header;
		memset(&icmp_header, 0, sizeof(icmp_header));
		icmp_header.type = ICMP_ECHO;
		icmp_header.un.echo.id = getpid();
		icmp_header.un.echo.sequence = i + 1;
		icmp_header.checksum = checksum(&icmp_header, sizeof(icmp_header));

		if (sendto(sockfd, &icmp_header, sizeof(icmp_header), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) <= 0) {
			// Failed to send ICMP packet
			continue;
		}

		++sent;
		clock_gettime(CLOCK_MONOTONIC, &time_start);
		struct sockaddr_in recv_addr;
		unsigned int addrlen = sizeof(recv_addr);
		char buffer[1500];
		if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&recv_addr, &addrlen) <= 0) {
			// Failed to receive ICMP packet
			continue;
		}

		++received;
		clock_gettime(CLOCK_MONOTONIC, &time_end);
		rtt += (double)(time_end.tv_sec - time_start.tv_sec) * 1000.0 + (double)(time_end.tv_nsec - time_start.tv_nsec) / 10;
	}

	clock_gettime(CLOCK_MONOTONIC, &tfe);
	double timeElapsed = (double)(tfe.tv_sec - tfs.tv_sec) * 1000.0 + (double)(tfe.tv_nsec - tfs.tv_nsec) / 1e6;

	close(sockfd);
	if (received == 0 || timeElapsed > timeout) {
		return -1.0;
	}
	return rtt / received;
}

size_t
CurlStringBufferWrite(void *contents, size_t size, size_t nmemb, std::string *s)
{
	size_t newLength = size * nmemb;
	try {
		s->append((char*)contents, newLength);
	} catch(std::bad_alloc &e) {
		return 0;
	}
	return newLength;
}

std::vector<std::string>
get_mirror_list(const char* url)
{
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "Error: Unable to init curl!\n");
		return {};
	}
	std::string resultBuffer;
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlStringBufferWrite);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resultBuffer);
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		curl_easy_cleanup(curl);
		return {};
	}
	curl_easy_cleanup(curl);
	std::string delimiter("\n");
	 std::vector<std::string> strings;
	 std::string::size_type pos = 0;
	 std::string::size_type prev = 0;
	 while ((pos = resultBuffer.find(delimiter, prev)) != std::string::npos)
	 {
		  strings.push_back(resultBuffer.substr(prev, pos - prev));
		  prev = pos + delimiter.size();
	 }
	 strings.push_back(resultBuffer.substr(prev));
	 return strings;
}

void
test_mirror(struct RepoMirror *mirror) {
	char *server;
	curl_url_get(mirror->url, CURLUPART_HOST, &server, 0);
	std::cout << "  * " << server << " (" << mirror->country << ")" << "...";

	// TODO: Quick access test of URL over https?

	mirror->metric = icmp_ping(server, 2, 5);
	std::cout << mirror->metric << " ms" << std::endl;
	curl_free(server);
}

struct RepoMirror
parse_mirror(const std::string &entry)
{
	 std::vector<std::string> tokens;
	 std::istringstream iss(entry);
	 std::string token;
	 while (std::getline(iss, token, ',')) {
		  tokens.push_back(token);
	 }

	 RepoMirror mirror{};
	 if (tokens.size() != 5) {
	 	return mirror;
	 }
	 mirror.url = curl_url();
	 CURLUcode result = curl_url_set(mirror.url, CURLUPART_URL, tokens.at(4).c_str(), 0);
	 if (result != CURLUE_OK)
	 	return RepoMirror{};
	 mirror.country = tokens.at(0);
	 mirror.city = tokens.at(1);
	 mirror.provider = tokens.at(2);
	 mirror.website = tokens.at(3);
	 mirror.metric = 0.0;
	 test_mirror(&mirror);
	return mirror;
}

int
main(int argc, char* argv[])
{
	std::vector<std::string> mirrorlist = get_mirror_list("https://eu.hpkg.haiku-os.org/haikuports/master/x86_64/current/mirrors.txt");
	std::cout << "Testing known mirrors..." << std::endl;

	std::vector<RepoMirror> mirrors;
	for (auto mirror : mirrorlist) {
		// If length is less than number of delim, or starts with a comment, skip
		if (mirror.size() < 4 || mirror.at(0) == '#')
			continue;
		mirrors.push_back(parse_mirror(mirror));
	};

	RepoMirror* bestMirror = NULL;
	for (auto mirror : mirrors) {
		if (bestMirror == NULL || (mirror.metric > 0.00 && mirror.metric < bestMirror->metric))
			bestMirror = &mirror;
	};

	char* url;
	CURLUcode result = curl_url_get(bestMirror->url, CURLUPART_URL, &url, CURLU_NO_DEFAULT_PORT);
	 if (result != CURLUE_OK) {
		fprintf(stderr, "curl_url_get() failed for best mirror! %d\n", result);
		return 1;
	}
	printf("Recommendation:	pkgman add-repo %s\n", url);
	return 0;
}
