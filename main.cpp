#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cstdio>


int main() {
    FILE* fp = std::fopen("example_new.ts", "rb");
    if (!fp) {
        std::perror("File opening failed");
        return -1;
    }

    FILE* out = std::fopen("output_audio.mp2", "wb");
    if (!out) {
        std::perror("File opening failed for output file");
        std::fclose(fp);
        return -1;
    }

    unsigned char packet[188];
    bool pesStarted = false;
    int pesHeaderDataLength = 0;
    int counter = 0;
    int printer = 20;

    while (std::fread(packet, sizeof(packet), 1, fp) == 1) {
        unsigned char syncByte = packet[0];                             //SB

        if (syncByte != 0x47) {
            std::cerr << "Sync byte error" << std::endl;
            continue;
        }
        bool tei = (packet[1] & 128) >> 7;                             //E
        bool psi = (packet[1] & 64) >> 6;                              //S
        bool tp = (packet[1] & 32) >> 5;                               //P
        uint16_t pid = (((packet[1] & 31) << 8) | packet[2]) & 0x1FFF; //PID
        unsigned char afc = (packet[3] & 48) >> 4;                     //AFC
        unsigned char tsc = (packet[3] & 192) >> 6;                    //TSC
        unsigned char cc = (packet[3] & 15);                           //CC
        if(counter < printer){
            printf("NO. %d TS : SB=%d E=%d S=%d P=%d PID=%d TSC=%d AF=%d CC=%d\n", counter, syncByte, tei, psi, tp, pid, tsc, afc, cc);
        }


        unsigned int payloadStart = 4;
        if (afc == 2 || afc == 3) {
            unsigned char afl = packet[4];                              //L
            bool dc = (packet[5] & 128) >> 7;                           //DC
            bool ra = (packet[5] & 64) >> 6;                            //RA
            bool sp = (packet[5] & 32) >> 5;                            //SP
            bool pr = (packet[5] & 16) >> 4;                            //PR
            bool ogr = (packet[5] & 8) << 3;                            //OR
            bool sf = (packet[5] & 4) << 2;                             //SF
            bool tpd = (packet[5] & 2) << 1;                            //TPD
            bool ex = (packet[5] & 1);                                  //EX
            if(counter < printer){
                printf("        AF : L=%d DC=%d RA=%d SP=%d PR=%d OR=%d SP=%d TP=%d EX=%d\n", afl, dc, ra, sp, pr, ogr, sf, tpd, ex);
            }


            payloadStart += 1 + afl;                                    // Skip adaptation field
        }

        if (pid == 136 && payloadStart < 188) {
            if (psi) {                                                  //payload unit start indicator
                if (packet[payloadStart] == 0x00 && packet[payloadStart + 1] == 0x00 && packet[payloadStart + 2] == 0x01) {
                    // PES start code detected
                    uint8_t sid = packet[payloadStart+3];
                    pesStarted = true;
                    unsigned char pesHeaderLength = packet[payloadStart + 8]; // Needed to skip the header
                    pesHeaderDataLength = 9 + pesHeaderLength;                //  Calculating the whole header
                    uint16_t l = (packet[payloadStart + 4] << 8) + packet[payloadStart + 5];
                    if(counter < printer){
                        printf("        PES: PSCP=1 SID=%d L=%d\n", sid, l);
                    }

                }
            }

            if (pesStarted) {
                if (psi) { //payload unit start indicator
                    // First packet of the PES contains the header
                    if (pesHeaderDataLength < 188 - payloadStart) {
                        std::fwrite(&packet[payloadStart + pesHeaderDataLength], 1, 188 - (payloadStart + pesHeaderDataLength), out);
                        pesHeaderDataLength = 0;  // Reset PES header length after the first packet
                        if(counter < printer){
                            printf("Started \n");
                        }

                    }
                }
                else {
                    // Subsequent packets contain only PES payload
                    std::fwrite(&packet[payloadStart], 1, 188 - payloadStart, out);
                    if(counter < printer){
                        printf("Continue \n");
                    }
                }
            }
        }
        counter++;
    }

    std::fclose(fp);
    std::fclose(out);
    std::cout << "Extraction complete." << std::endl;

    return 0;
}
