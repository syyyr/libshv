#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <shv/coreqt/log.h>
#include <shv/visu/timeline/graph.h>
#include <shv/visu/timeline/graphmodel.h>
#include <shv/visu/timeline/graphwidget.h>

#include <random>

using namespace std;
using namespace shv::chainpack;
namespace tl = shv::visu::timeline;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	ui->btGenerateSamples->setDefaultAction(ui->actionGenerate_sample_data);

	m_graphModel = new tl::GraphModel(this);
	m_graphWidget = new tl::GraphWidget();

	ui->graphView->setBackgroundRole(QPalette::Dark);
	ui->graphView->setWidget(m_graphWidget);
	ui->graphView->widget()->setBackgroundRole(QPalette::ToolTipBase);
	ui->graphView->widget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	m_graph = new tl::Graph(this);
	tl::Graph::Style graph_style = m_graph->style();
	//graph_style.setYAxisVisible(true);
	m_graph->setStyle(graph_style);
	tl::GraphChannel::Style channel_style = m_graph->defaultChannelStyle();
	//channel_style.setColorGrid(QColor());
	m_graph->setDefaultChannelStyle(channel_style);
	m_graph->setModel(m_graphModel);
	m_graphWidget->setGraph(m_graph);
	generateSampleData();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::generateSampleData()
{
	int sample_cnt = ui->samplesCount->value();
	int64_t min_time = 0;
	int64_t max_time = 1000LL * 60 * 60 * 24 * 365;
	double min_val = -3;
	double max_val = 5;

	m_graphModel->clear();
	const vector<string>paths{"discrete", "stepped", "line"};
	for (const auto &ch : paths) {
		m_graphModel->appendChannel(ch, std::string());
	}
	//m_graphModel->appendChannel("Discrete", std::string());

	m_graphModel->beginAppendValues();

	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<> time_distrib(min_time, max_time);
	std::uniform_real_distribution<> val_distrib(min_val, max_val);
	vector<int64_t> times;
	for (int n=0; n<sample_cnt; ++n)
		times.push_back(time_distrib(gen));
	sort(times.begin(), times.end());
	for(auto time : times) {
		double val = val_distrib(gen);
		for (int i=0; i<m_graphModel->channelCount(); i++) {
			m_graphModel->appendValue(i, tl::Sample{time, val});
		}
	}

	m_graphModel->endAppendValues();

	m_graph->createChannelsFromModel();
	for (int i = 0; i < m_graph->channelCount(); ++i) {
		shv::visu::timeline::GraphChannel *ch = m_graph->channelAt(i);
		shv::visu::timeline::GraphChannel::Style style = ch->style();
		if(ch->shvPath() == "line") {
			style.setInterpolation(tl::GraphChannel::Style::Interpolation::Line);
			style.setLineAreaStyle(tl::GraphChannel::Style::LineAreaStyle::Filled);
		}
		else if(ch->shvPath() == "discrete") {
			style.setInterpolation(tl::GraphChannel::Style::Interpolation::None);
			style.setLineAreaStyle(tl::GraphChannel::Style::LineAreaStyle::Filled);
		}
		else {
			style.setInterpolation(tl::GraphChannel::Style::Interpolation::Stepped);
			style.setLineAreaStyle(tl::GraphChannel::Style::LineAreaStyle::Filled);
		}
		ch->setStyle(style);
	}
	ui->graphView->makeLayout();
}


void MainWindow::on_actionGenerate_sample_data_triggered()
{
	generateSampleData();
}
