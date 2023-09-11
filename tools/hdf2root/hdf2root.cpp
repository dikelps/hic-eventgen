#include <highfive/H5File.hpp>
#include <highfive/H5DataType.hpp>

#include "TFile.h"
#include "TTree.h"

#include <string>
#include <math.h>

using namespace HighFive;

static const std::string FILE_NAME("trento.h5");
static const std::string FILE_NAME_URQMD("urqmd.h5");

enum
{
    nTrkMax = 200000,
    NT = 6
};

const double etaMax = 5.00; // eta cut 5.0
const int NETA = 1000;

const double etaMax1 = 2.00; // eta cut 2.0
const int NETA1 = 400;       // 400 bins

const int NPHI = 512;
const double PI = acos(-1.0);
const double PI2 = 2 * PI;
const double PHIBIN = PI2 / NPHI;

const double ptMin = 0.1;
const int NPT = 512;

const int NHAR = 5;
const int NMAX = 1000; // 238 +238; //U+U

float xx[nTrkMax], yy[nTrkMax], ww[nTrkMax];
float w[nTrkMax], xm, ym, sumpt, sumpt1, wm;
int Np[NT], flag[NT]; // 0 total Npart 1 forward Npart 2 backward Npart

typedef struct
{
    long sample;
    long ID;
    long charge;
    double pT;
    double ET;
    double mT;
    double phi;
    double y;
    double eta;
} urqmd_evt;

CompoundType create_compound()
{
    // return {{"u1", create_datatype<logn>()},
    //         {"u2", create_datatype<short>()},
    //         {"u3", create_datatype<unsigned long long>()}};
    return {
        {"sample", create_datatype<long>()},
        {"ID", create_datatype<long>()},
        {"charge", create_datatype<long>()},
        {"pT", create_datatype<double>()},
        {"ET", create_datatype<double>()},
        {"mT", create_datatype<double>()},
        {"phi", create_datatype<double>()},
        {"y", create_datatype<double>()},
        {"eta", create_datatype<double>()}};
}
HIGHFIVE_REGISTER_TYPE(urqmd_evt, create_compound)
int main(int argc, char **argv)
{
    try
    {
        // arg 1 trento-only or not
        bool trento_only = false;
        if (argc > 1)
        {
            std::string arg1 = argv[1];
            if (arg1 == "trento_only")
            {
                trento_only = true;
            }
        }

        if (trento_only)
        {
            File file(FILE_NAME, File::ReadOnly);
            TFile fout("out.root", "recreate");
            TTree tree("tree", "");
            float re[NHAR]; // npart, nA, nB
            float rang[NHAR];
            float frcos[NHAR];
            float frsin[NHAR];
            float fr[NHAR];
            float pe[NHAR];
            float pang[NHAR];
            double xmn[2], ymn[2], xymn[2]; // this used for transverse area
            float radius[2];
            float ep2; // RP epsilon_2
            float bimp;
            int npart;
            float mult;
            float varX;
            float varY;
            float varXY;

            // event level information
            tree.Branch("b", &bimp, "b/F");
            tree.Branch("npart", &npart, "npart/I");
            tree.Branch("mult", &mult, "mult/F");
            tree.Branch("re", &re, "re[5]/F");
            tree.Branch("rang", &rang, "rang[5]/F");
            tree.Branch("VarX", &varX, "varX/F");
            tree.Branch("VarY", &varY, "varY/F");
            tree.Branch("VarXY", &varXY, "varXY/F");

            std::map<std::string, std::string> attr_names{};
            attr_names["b"] = "b";
            attr_names["npart"] = "npart";
            attr_names["mult"] = "mult";
            attr_names["nsample"] = "nsample";

            size_t nevt = file.getNumberObjects();
            ;

            for (size_t ievt = 0; ievt < nevt; ievt++) // event loop starts
            {
                std::string iname = "event_" + std::to_string(ievt);

                // trento data
                auto d = file.getDataSet(iname);
                std::vector<std::vector<double>> result;
                // read all trentoto data into a single vector
                d.read(result);
                d.getAttribute(attr_names["b"]).read(bimp);
                d.getAttribute(attr_names["mult"]).read(mult);
                d.getAttribute(attr_names["npart"]).read(npart);

                xm = ym = 0;
                wm = 0;
                xmn[0] = xmn[1] = 0;
                ymn[0] = ymn[1] = 0;
                xymn[0] = xymn[1] = 0;

                size_t npcounter = 0;
                size_t idy = 0;

                // the cell size in the entropy map
                // total size is 30x30 fm
                float cell_size = 30.0 / result.size();
                for (auto i : result)
                {
                    idy++;
                    size_t idx = 0;
                    for (auto j : i)
                    {
                        idx++;
                        double ypy = idy * cell_size;
                        double ypx = idx * cell_size;
                        double weight = j;
                        // this is used for size calculation
                        xmn[0] += weight * ypx;
                        xmn[1] += weight * ypx * ypx;
                        ymn[0] += weight * ypy;
                        ymn[1] += weight * ypy * ypy;
                        xymn[0] += weight * ypx * ypy;
                        xymn[1] += weight * (ypx * ypx + ypy * ypy);
                        // prepare for recentering
                        xx[npcounter] = ypx;
                        yy[npcounter] = ypy;
                        ww[npcounter] = weight;
                        // prepare for mean x and mean y
                        xm += weight * ypx;
                        ym += weight * ypy;
                        wm += weight;
                        npcounter++;
                    }
                }
                // center of mass
                xm /= wm;
                ym /= wm;

                for (int it = 0; it < 2; it++)
                {
                    xmn[it] = xmn[it] / wm;
                    ymn[it] = ymn[it] / wm;
                    xymn[it] = xymn[it] / wm;
                }
                varX = xmn[1] - (xmn[0] * xmn[0]);
                varY = ymn[1] - (ymn[0] * ymn[0]);
                varXY = xymn[0] - (xmn[0] * ymn[0]);
                radius[0] = pow((xmn[1] - xmn[0] * xmn[0]) * (ymn[1] - ymn[0] * ymn[0]) - pow(xymn[0] - xmn[0] * ymn[0], 2), 1. / 2); //<(sigma_x^2*sigma_y^2 - sigma_xy^2)^{1./2}> arXiv0904.4080
                radius[1] = xmn[1] + ymn[1] - xmn[0] * xmn[0] - ymn[0] * ymn[0];                                                      // arXiv:1701.09105v1
                double sx = xmn[1] - xmn[0] * xmn[0];
                double sy = ymn[1] - ymn[0] * ymn[0];
                ep2 = (sy - sx) / (sy + sx); // Epsilon_RP;

                for (int ihar = 0; ihar < NHAR; ihar++)
                {
                    frcos[ihar] = 0;
                    frsin[ihar] = 0;
                    fr[ihar] = 0;
                }
                for (int ip = 0; ip < npcounter; ip++)
                {
                    double rr = sqrt(pow(xx[ip] - xm, 2) + pow(yy[ip] - ym, 2));
                    double phi = atan2(yy[ip] - ym, xx[ip] - xm);
                    double r = 0;
                    for (int ihar = 0; ihar < NHAR; ihar++)
                    {
                        if (ihar == 0)
                            r = pow(rr, 3);
                        else
                            r = pow(rr, ihar + 1);
                        frcos[ihar] += ww[ip] * r * cos((ihar + 1) * phi);
                        frsin[ihar] += ww[ip] * r * sin((ihar + 1) * phi);
                        fr[ihar] += ww[ip] * r;
                    }
                }

                for (int ihar = 0; ihar < NHAR; ihar++)
                {
                    // harmonic angles
                    rang[ihar] = (atan2(frsin[ihar], frcos[ihar]) + PI) / double(ihar + 1);
                    if (rang[ihar] < -PI / (ihar + 1))
                        rang[ihar] += PI2 / (ihar + 1);
                    if (rang[ihar] > PI / (ihar + 1))
                        rang[ihar] -= PI2 / (ihar + 1);
                    // harmonic eccentricities
                    re[ihar] = sqrt(pow(frcos[ihar], 2) + pow(frsin[ihar], 2)) / fr[ihar];
                }
                tree.Fill();
            }
            tree.Write();
            fout.Close();

            return 0; // successfully terminated
        }
        else
        {
            File file(FILE_NAME, File::ReadOnly);
            TFile fout("out.root", "recreate");
            TTree tree("tree", "");
            float re[NHAR]; // npart, nA, nB
            float rang[NHAR];
            float frcos[NHAR];
            float frsin[NHAR];
            float fr[NHAR];
            float pe[NHAR];
            float pang[NHAR];
            double xmn[2], ymn[2], xymn[2]; // this used for transverse area
            float radius[2];
            float ep2; // RP epsilon_2
            float bimp;
            int npart;
            float mult;
            float varX;
            float varY;
            float varXY;

            // event level information
            tree.Branch("b", &bimp, "b/F");
            tree.Branch("npart", &npart, "npart/I");
            tree.Branch("mult", &mult, "mult/F");
            tree.Branch("re", &re, "re[5]/F");
            tree.Branch("rang", &rang, "rang[5]/F");
            tree.Branch("VarX", &varX, "varX/F");
            tree.Branch("VarY", &varY, "varY/F");
            tree.Branch("VarXY", &varXY, "varXY/F");

            std::map<std::string, std::string> attr_names{};
            attr_names["b"] = "b";
            attr_names["npart"] = "npart";
            attr_names["mult"] = "mult";
            attr_names["nsample"] = "nsample";

            // URQMD data
            File file_urqmd(FILE_NAME_URQMD, File::ReadOnly);

            CompoundType u_packed({{{"sample", create_datatype<long>(), 0},
                                    {"ID", create_datatype<long>(), 8},
                                    {"charge", create_datatype<long>(), 16},
                                    {"pT", create_datatype<double>(), 24},
                                    {"ET", create_datatype<double>(), 32},
                                    {"mT", create_datatype<double>(), 40},
                                    {"phi", create_datatype<double>(), 48},
                                    {"y", create_datatype<double>(), 56},
                                    {"eta", create_datatype<double>(), 64}}},
                                  72);

            int nTrk;
            char trkID[nTrkMax];
            float trkPt[nTrkMax];
            short trkEta[nTrkMax];
            short trkPhi[nTrkMax];
            char trkCharge[nTrkMax];
            int trkSampleId[nTrkMax];

            // track level information
            tree.Branch("nTrk", &nTrk, "nTrk/I");
            tree.Branch("id", trkID, "id[nTrk]/B");
            tree.Branch("pt", trkPt, "pt[nTrk]/F");
            tree.Branch("eta", trkEta, "eta[nTrk]/S"); // 0-400
            tree.Branch("phi", trkPhi, "phi[nTrk]/S"); // 0-512
            tree.Branch("charge", trkCharge, "charge[nTrk]/B");
            tree.Branch("sampleid", trkSampleId, "sampleid[nTrk]/I");

            size_t ievt = 0;
            auto nevt = file_urqmd.getNumberObjects();
            for (size_t ievt = 0; ievt < nevt; ievt++) // event loop starts
            {
                // if (ievt > 10)
                // continue;
                std::string iname = "event_" + std::to_string(ievt);
                // std::cout << iname << std::endl;

                // trento data
                auto d = file.getDataSet(iname);
                std::vector<std::vector<double>> result;
                // read all trentoto data into a single vector
                d.read(result);
                d.getAttribute(attr_names["b"]).read(bimp);
                d.getAttribute(attr_names["mult"]).read(mult);
                d.getAttribute(attr_names["npart"]).read(npart);

                // urqmd data
                std::vector<urqmd_evt> vecevt;
                std::string iname_urqmd = "event_" + std::to_string(ievt);
                DataSet d_urqmd = file_urqmd.getDataSet(iname_urqmd);
                d_urqmd.read(vecevt);
                size_t nelement = d_urqmd.getElementCount();
                // size_t nelement = vecevt.size();

                if (vecevt.size() != nelement)
                {
                    std::cout << "track size mismatch between dataset and vector.\n";
                }
                // std::cout << "number of elements: " << nelement << std::endl;
                nTrk = 0;
                if (nelement > nTrkMax)
                {
                    std::cout << "Particle number exceeds nTrkMax, skipped event to prevent array overflow. \n";
                    continue;
                }

                for (int itrk = 0; itrk < nelement; itrk++)
                {
                    long samplen = vecevt.at(itrk).sample;
                    long i0 = vecevt.at(itrk).ID;
                    if (abs(i0) < 50)
                        continue; // reject leptions and photons
                    if (abs(i0) > 3000)
                        continue; // reject high mass baryons, sigma, etc
                    int type = 0;
                    switch (i0)
                    {
                    case 211:
                        type = 0;
                        break; // pi+
                    case 321:
                        type = 1;
                        break; // k+
                    case 2212:
                        type = 2;
                        break; // p

                    case -211:
                        type = 3;
                        break; // pi-
                    case -321:
                        type = 4;
                        break; // k-
                    case -2212:
                        type = 5;
                        break; // pbar

                    case 111:
                        type = 6;
                        break; // pi0
                    case 221:
                        type = 7;
                        break; // eta

                    case 130:
                    case 310:
                        type = 8;
                        break; // K0

                    case 2112:
                        type = 9;
                        break; // n
                    case -2112:
                        type = 10;
                        break; // nbar
                    default:
                        type = 11; // this should not happen
                    };
                    if (type == 11)
                        continue;

                    double eta = vecevt.at(itrk).eta;
                    if (fabs(eta) >= etaMax1)
                        continue;
                    double pt = vecevt.at(itrk).pT;
                    if (pt < ptMin)
                        continue;
                    double phi = vecevt.at(itrk).phi;

                    trkSampleId[nTrk] = samplen;
                    trkEta[nTrk] = (eta / etaMax1 + 1.0) * NETA1 / 2.0;
                    // phi goes from -pi to pi, add pi to make it from 0 to 2pi
                    trkPhi[nTrk] = (phi + PI) / PHIBIN;
                    // std::cout << "actual phi:" << phi +PI << "\t"  << "converted phi:" << trkPhi[nTrk] << std::endl;
                    trkPt[nTrk] = pt;
                    trkID[nTrk] = type;
                    long i1 = vecevt.at(itrk).charge;

                    trkCharge[nTrk] = i1;

                    // std::cout <<vecevt.at(itrk).charge << "\t" << trkCharge[nTrk] << "\t" << i1 << std::endl;
                    nTrk++;
                }
                if (nTrk == 0)
                {
                    continue;
                }

                xm = ym = 0;
                wm = 0;
                xmn[0] = xmn[1] = 0;
                ymn[0] = ymn[1] = 0;
                xymn[0] = xymn[1] = 0;

                size_t npcounter = 0;
                size_t idy = 0;

                // the cell size in the entropy map
                // total size is 30x30 fm
                float cell_size = 30.0 / result.size();
                for (auto i : result)
                {
                    idy++;
                    size_t idx = 0;
                    for (auto j : i)
                    {
                        idx++;
                        double ypy = idy * cell_size;
                        double ypx = idx * cell_size;
                        double weight = j;
                        // this is used for size calculation
                        xmn[0] += weight * ypx;
                        xmn[1] += weight * ypx * ypx;
                        ymn[0] += weight * ypy;
                        ymn[1] += weight * ypy * ypy;
                        xymn[0] += weight * ypx * ypy;
                        xymn[1] += weight * (ypx * ypx + ypy * ypy);
                        // prepare for recentering
                        xx[npcounter] = ypx;
                        yy[npcounter] = ypy;
                        ww[npcounter] = weight;
                        // prepare for mean x and mean y
                        xm += weight * ypx;
                        ym += weight * ypy;
                        wm += weight;
                        npcounter++;
                    }
                }
                // center of mass
                xm /= wm;
                ym /= wm;

                for (int it = 0; it < 2; it++)
                {
                    xmn[it] = xmn[it] / wm;
                    ymn[it] = ymn[it] / wm;
                    xymn[it] = xymn[it] / wm;
                }
                varX = xmn[1] - (xmn[0] * xmn[0]);
                varY = ymn[1] - (ymn[0] * ymn[0]);
                varXY = xymn[0] - (xmn[0] * ymn[0]);
                radius[0] = pow((xmn[1] - xmn[0] * xmn[0]) * (ymn[1] - ymn[0] * ymn[0]) - pow(xymn[0] - xmn[0] * ymn[0], 2), 1. / 2); //<(sigma_x^2*sigma_y^2 - sigma_xy^2)^{1./2}> arXiv0904.4080
                radius[1] = xmn[1] + ymn[1] - xmn[0] * xmn[0] - ymn[0] * ymn[0];                                                      // arXiv:1701.09105v1
                double sx = xmn[1] - xmn[0] * xmn[0];
                double sy = ymn[1] - ymn[0] * ymn[0];
                ep2 = (sy - sx) / (sy + sx); // Epsilon_RP;

                for (int ihar = 0; ihar < NHAR; ihar++)
                {
                    frcos[ihar] = 0;
                    frsin[ihar] = 0;
                    fr[ihar] = 0;
                }
                for (int ip = 0; ip < npcounter; ip++)
                {
                    double rr = sqrt(pow(xx[ip] - xm, 2) + pow(yy[ip] - ym, 2));
                    double phi = atan2(yy[ip] - ym, xx[ip] - xm);
                    double r = 0;
                    for (int ihar = 0; ihar < NHAR; ihar++)
                    {
                        if (ihar == 0)
                            r = pow(rr, 3);
                        else
                            r = pow(rr, ihar + 1);
                        frcos[ihar] += ww[ip] * r * cos((ihar + 1) * phi);
                        frsin[ihar] += ww[ip] * r * sin((ihar + 1) * phi);
                        fr[ihar] += ww[ip] * r;
                    }
                }

                for (int ihar = 0; ihar < NHAR; ihar++)
                {
                    // harmonic angles
                    rang[ihar] = (atan2(frsin[ihar], frcos[ihar]) + PI) / double(ihar + 1);
                    if (rang[ihar] < -PI / (ihar + 1))
                        rang[ihar] += PI2 / (ihar + 1);
                    if (rang[ihar] > PI / (ihar + 1))
                        rang[ihar] -= PI2 / (ihar + 1);
                    // harmonic eccentricities
                    re[ihar] = sqrt(pow(frcos[ihar], 2) + pow(frsin[ihar], 2)) / fr[ihar];
                }
                tree.Fill();
            }
            tree.Write();
            fout.Close();
        }
    }
    catch (const Exception &err)
    {
        // catch and print any HDF5 error
        std::cerr << err.what() << std::endl;
        return 1;
    }
    return 0; // successfully terminated
}