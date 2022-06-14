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
	static constexpr int64_t YEAR = 1000LL * 60 * 60 * 24 * 365;
	int64_t min_time = 100;
	int64_t max_time = min_time + YEAR;
	double min_val = -3;
	double max_val = 5;

	m_graphModel->clear();
	const vector<string>paths{"discrete", "stepped", "line", "withMissingData"};
	for (const auto &ch : paths) {
		m_graphModel->appendChannel(ch, std::string());
	}
	//m_graphModel->appendChannel("Discrete", std::string());

	m_graphModel->beginAppendValues();

	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<int64_t> time_distrib(min_time, max_time);
	std::uniform_real_distribution<> val_distrib(min_val, max_val);

	vector<int64_t> times;
	for (int n=0; n<sample_cnt+2; ++n)
		times.push_back(time_distrib(gen));
	sort(times.begin(), times.end());
	m_graphModel->appendValue(0, tl::Sample{times[0], val_distrib(gen)});
	for(size_t j=1; j<times.size()-1; ++j) {
		for (int i=0; i<m_graphModel->channelCount() - 1; i++) {
			if(j == 3) {
				for(int k=0; k<5; k++)
					m_graphModel->appendValue(i, tl::Sample{times[j]+k, val_distrib(gen)});
			}
			else {
				m_graphModel->appendValue(i, tl::Sample{times[j], val_distrib(gen)});
			}
		}
	}
	m_graphModel->appendValue(0, tl::Sample{times[times.size()-1], val_distrib(gen)});
	{
		// generate data with not available part in center third
		bool na_added = false;
		for(size_t j=1; j<times.size()-1; ++j) {
			if(j > times.size()/3 && j < times.size()/3*2) {
				if(!na_added) {
					na_added = true;
					m_graphModel->appendValue(m_graphModel->channelCount() - 1, tl::Sample{times[j], QVariant()});
				}
			}
			else {
				m_graphModel->appendValue(m_graphModel->channelCount() - 1, tl::Sample{times[j], val_distrib(gen)});
			}
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
		//style.setInterpolation(tl::GraphChannel::Style::Interpolation::None);
		ch->setStyle(style);
	}
	ui->graphView->makeLayout();
}


void MainWindow::on_actionGenerate_sample_data_triggered()
{
	generateSampleData();
}
