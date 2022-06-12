#include "BlenderProcess.h"
#include "GeometryNodesPlugin.h"

#include <QFileInfo>
//#include <QBuffer>

namespace GeometryNodes
{
	BlenderProcess::BlenderProcess(QString id, QObject* parent) : QProcess(parent)
	{
		connect(this, &BlenderProcess::readyReadStandardOutput, this, &BlenderProcess::readResponse);
	}

	bool BlenderProcess::run(Unigine::String abspath)
	{
		auto pypath = GeometryNodesPlugin::get()->getPythonPath();
		if (!QFileInfo::exists(pypath))
		{
			Unigine::Log::error("Not found:\n%s\n", pypath.toUtf8().constData());
			return false;
		}

		QString blender = GeometryNodesPlugin::get()->getBlenderPath();

		QStringList args = QStringList() << "-b" << abspath.get() << "-P" << pypath;
		start(blender, args);
		if (!this->waitForStarted(1000))
		{
			Unigine::Log::error("Unable to run Blender:\n%s\nWrite '%' in the console to find Blender\nor use the '%s' command to specify the path manually\n", blender.toUtf8().constData(), BLENDER_FIND_CMD, BLENDER_PATH_CFG);
			return false;
		}
		this->setTextModeEnabled(false);
		return true;
	}

	void BlenderProcess::readResponse()
	{
		//Timer tm;
		//tm.begin();
		auto output = this->readAllStandardOutput();
		//tm.end();
		//Log::message("read stdin took %f\n", tm.endSeconds());

		if (output.size() < 9) {
			state_ = State::IDLE;
			Unigine::Log::message("%s\n", output.constData());
			return;
		}

		QDataStream in(output);
		in.setByteOrder(QDataStream::LittleEndian);
		in.setFloatingPointPrecision(QDataStream::FloatingPointPrecision::SinglePrecision);

		int cmdLength = 9;
		QByteArray rawCmd;
		QString cmd;
		rawCmd.resize(9);
		if (in.readRawData(rawCmd.data(), cmdLength) == cmdLength) {
			cmd = QString::fromLatin1(rawCmd);
		}

		if (cmd == "BGN_READY")
		{
			// started
			state_ = State::READY;
			//request("params");
		}
		else if (cmd == "BGN_PARAM")
		{
			state_ = State::PARAMS;
			in >> params_;
		}
		else if (cmd == "BGN_MESHS")
		{
			state_ = State::MESH;

			if (pendingChanges_.params.size())
			{
				requestMesh(pendingChanges_.params.values(), pendingChanges_.hashes);
				pendingChanges_.clear();
			}

			//tm.begin();
			in >> meshdata_;
			//tm.end();
			//Log::message("read meshdata %f\n", tm.endSeconds());

		}
		else
		{
			// unknown
			state_ = State::IDLE;
			Unigine::Log::message("%s\n", output.constData());
		}
	}

	Unigine::Vector<GNParam> BlenderProcess::getParameters()
	{
		state_ = State::IDLE;
		return params_;
	}

	GNData BlenderProcess::getMeshData()
	{
		state_ = State::IDLE;
		return meshdata_;
	}

	void BlenderProcess::request(const char* str)
	{
		this->request_b(tr(str).toLatin1());
	}

	void BlenderProcess::request_b(QByteArray data)
	{
		if (state_ == State::BUSY) { return; }
		state_ = State::BUSY;
		this->write(data);
	}

	void BlenderProcess::requestMesh(Unigine::Vector<GNParam> params, Unigine::Vector<long long> hashes)
	{
		if (state_ == State::BUSY)
		{
			for (auto& p : params)
			{
				pendingChanges_.params.append(p.id, p);
			}
			pendingChanges_.hashes = hashes;
			return;
		}

		QByteArray data;
		QDataStream out(&data, QIODevice::ReadWrite);
		out.setByteOrder(QDataStream::LittleEndian);
		out.setFloatingPointPrecision(QDataStream::FloatingPointPrecision::SinglePrecision);

		out.writeRawData("mesh", 4);
		out << hashes;
		out << params;

		request_b(data);
	}

	void BlenderProcess::updateParameter(GNParam& param, Unigine::Vector<long long> hashes, bool sync)
	{
		Unigine::Vector<GNParam> params;
		params.append(param);
		requestMesh(params, hashes);
		if (sync)
			waitForReadyRead(2000);
	}

	/*void BlenderProcess::readResponseFromSharedMemory()
	{
		auto output = this->readAllStandardOutput();

		if (output.size() < 9) {
			//state_ = State::IDLE;
			Log::message("unknown response: %s\n", output.constData());
			return;
		}

		QDataStream in(output);
		in.setByteOrder(QDataStream::LittleEndian);
		in.setFloatingPointPrecision(QDataStream::FloatingPointPrecision::SinglePrecision);

		char *cmd = new char[10];
		cmd[9] = '\0';
		in.readRawData(cmd, 9);

		//auto str = QString::fromLatin1(output.left(9));
		//if (str == "BGN_READY")
		if (strcmp(cmd, "BGN_READY") == 0)
		{
			// started
			state_ = State::READY;
			//request("params");

			//sharedMemory_.detach();
		}
		//else if (str == "BGN_PARAM" || str == "BGN_MESHS")
		else if (strcmp(cmd, "BGN_PARAM") == 0 || strcmp(cmd, "BGN_MESHS") == 0)
		{
			//buffer_ = output.mid(9);

			QBuffer buffer;
			QDataStream inn(&buffer);
			inn.setByteOrder(QDataStream::LittleEndian);
			inn.setFloatingPointPrecision(QDataStream::FloatingPointPrecision::SinglePrecision);

			sharedMemory_.lock();
			buffer.setData((char*)sharedMemory_.constData(), sharedMemory_.size());
			buffer.open(QBuffer::ReadOnly);

			if (strcmp(cmd, "BGN_PARAM") == 0)
			{
				state_ = State::PARAMS;
				inn >> params_;
			}
			else
			{
				state_ = State::MESH;
				inn >> meshdata_;
			}
			sharedMemory_.unlock();
		}
		else
		{
			// unknown
			//state_ = State::IDLE;
			Log::message("%s\n", output.constData());
		}
		delete[] cmd;
	}*/
} // namespace GeometryNodes