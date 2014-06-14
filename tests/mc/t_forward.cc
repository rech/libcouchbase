#include "mctest.h"
#include "mc/mcreq-flush-inl.h"
#include "mc/forward.h"
#include "pktmaker.h"

using namespace PacketMaker;
using std::vector;
using std::string;

struct Vars {
    mc_PACKET *pkt;
    mc_PIPELINE *pl;
    nb_IOV iovs[10];
    mc_IOVINFO ioi;
    vector<char> reqbuf;

    Vars() {
        pkt = NULL;
        pl = NULL;
        memset(&ioi, 0, sizeof(ioi));
        memset(&iovs, 0, sizeof(iovs));
    }

    lcb_error_t requestPacket(mc_CMDQUEUE *cq) {
        return mc_forward_packet(cq, &ioi, &pkt, &pl, 0);
    }

    void initInfo() {
        mc_iovinfo_init(&ioi, iovs, 10);
    }
};

class McFwd : public ::testing::Test {};

static void
setupRequestBuf(vector<char>& out, size_t nkey, size_t nval)
{
    string k(nkey, 'K');
    string v(nval, 'V');
    StorageRequest sr(k, v);
    out.clear();
    sr.serialize(out);
    EXPECT_EQ(nkey + nval + 24, out.size());
}

TEST_F(McFwd, testForwardSingle)
{
    CQWrap cq;
    StorageRequest sr(string("fookey"), string("foovalue"));

    mc_IOVINFO iovinfo;

    nb_IOV iovs[10];
    // Enqueue first packet inside entire body.
    vector<char> reqbody;
    sr.serialize(reqbody);

    memset(iovs, 0, sizeof(iovs));
    memset(&iovinfo, 0, sizeof(iovinfo));
    mc_iovinfo_init(&iovinfo, iovs, 10);
    ASSERT_EQ(10, iovinfo.c.niov);
    ASSERT_EQ(&iovs[0], iovinfo.c.iov);
    ASSERT_NE(0, reqbody.size());

    iovs->iov_base = &reqbody[0];
    iovs->iov_len = reqbody.size();
    iovinfo.total = reqbody.size();

    mc_PACKET *pkt = NULL;
    mc_PIPELINE *pl = NULL;
    lcb_error_t rc = mc_forward_packet(&cq, &iovinfo, &pkt, &pl, 0);
    ASSERT_EQ(LCB_SUCCESS, rc);
    ASSERT_EQ(0, iovinfo.wanted);
    ASSERT_EQ(reqbody.size(), iovinfo.consumed);
    ASSERT_EQ(9, iovinfo.c.niov);
    ASSERT_EQ(0, iovinfo.c.offset);
    mcreq_sched_fail(&cq);
}

TEST_F(McFwd, testFragmentedBasic)
{
    CQWrap cq;
    nb_IOV iovs[10];
    vector<char> reqbuf;

    memset(iovs, 0, sizeof(iovs));
    setupRequestBuf(reqbuf, 10, 10);

    iovs[0].iov_base = &reqbuf[0];
    iovs[0].iov_len = 34;

    iovs[1].iov_base = &reqbuf[34];
    iovs[1].iov_len = 10;

    mc_IOVINFO ioi;
    memset(&ioi, 0, sizeof(ioi));
    mc_iovinfo_init(&ioi, iovs, 10);
    lcb_error_t rc;
    mc_PACKET *pkt;
    mc_PIPELINE *pl;

    rc = mc_forward_packet(&cq, &ioi, &pkt, &pl, 0);
    ASSERT_EQ(LCB_SUCCESS, rc);
    ASSERT_EQ(0, ioi.wanted);
    ASSERT_EQ(44, ioi.consumed);
    ASSERT_EQ(0, ioi.c.offset);
    ASSERT_EQ(8, ioi.c.niov);
    ASSERT_EQ(0, ioi.c.iov[0].iov_len);
    mcreq_sched_fail(&cq);
}

TEST_F(McFwd, testFragmentedHeader) {
    CQWrap cq;
    Vars vars;

    setupRequestBuf(vars.reqbuf, 100, 100);
    vars.iovs[0].iov_base = &vars.reqbuf[0];
    vars.iovs[0].iov_len = 10;

    vars.iovs[1].iov_base = &vars.reqbuf[10];
    vars.iovs[1].iov_len = 10;

    vars.iovs[2].iov_base = &vars.reqbuf[20];
    vars.iovs[2].iov_len = vars.reqbuf.size() - 20;
    vars.initInfo();
    ASSERT_EQ(vars.reqbuf.size(), vars.ioi.total);

    lcb_error_t rc = vars.requestPacket(&cq);
    ASSERT_EQ(LCB_SUCCESS, rc);
    ASSERT_EQ(0, vars.pkt->flags & MCREQ_F_KEY_NOCOPY);
    ASSERT_EQ(0, vars.ioi.total);
    ASSERT_EQ(0, vars.ioi.c.offset);
    ASSERT_EQ(vars.reqbuf.size(), vars.ioi.consumed);
    ASSERT_EQ(0, vars.ioi.c.iov[0].iov_len);
    ASSERT_EQ(7, vars.ioi.c.niov);

    mcreq_sched_fail(&cq);
}

TEST_F(McFwd, testInsufficientHeader)
{
    CQWrap cq;
    Vars vars;
    lcb_error_t rc;

    setupRequestBuf(vars.reqbuf, 100, 100);

    // Test with no data
    vars.iovs[0].iov_base = NULL;
    vars.iovs[0].iov_len = 0;
    vars.initInfo();
    rc = vars.requestPacket(&cq);
    ASSERT_EQ(LCB_INCOMPLETE_PACKET, rc);
    ASSERT_EQ(24, vars.ioi.wanted);

    // Test with partial (but incomplete header)
    vars.iovs[0].iov_base = &vars.reqbuf[0];
    vars.iovs[0].iov_len = 20;
    vars.initInfo();
    rc = vars.requestPacket(&cq);
    ASSERT_EQ(LCB_INCOMPLETE_PACKET, rc);
    ASSERT_EQ(24, vars.ioi.wanted);

    // Test with full header but partial key
    vars.iovs[0].iov_base = &vars.reqbuf[0];
    vars.iovs[0].iov_len = 30;
    vars.initInfo();
    rc = vars.requestPacket(&cq);
    ASSERT_EQ(rc, LCB_INCOMPLETE_PACKET);
    ASSERT_EQ(vars.reqbuf.size(), vars.ioi.wanted);
}

TEST_F(McFwd, testMultiValue)
{
    CQWrap cq;
    Vars vars;
    lcb_error_t rc;
    setupRequestBuf(vars.reqbuf, 1, 810);

    vars.iovs[0].iov_base = &vars.reqbuf[0];
    vars.iovs[0].iov_len = 25;

    for (int ii = 1; ii < 10; ii++) {
        vars.iovs[ii].iov_base = &vars.reqbuf[25 + (ii-1) * 90];
        vars.iovs[ii].iov_len = 90;
    }

    vars.initInfo();
    ASSERT_EQ(835, vars.reqbuf.size());
    ASSERT_EQ(835, vars.ioi.total);

    rc = vars.requestPacket(&cq);
    ASSERT_EQ(LCB_SUCCESS, rc);
    ASSERT_NE(0, vars.pkt->flags & MCREQ_F_VALUE_IOV);
    mcreq_sched_fail(&cq);

    // Eh, let's check these other nifty things. Why not?
    ASSERT_EQ(0, vars.ioi.wanted);
    ASSERT_EQ(0, vars.ioi.c.niov);
}