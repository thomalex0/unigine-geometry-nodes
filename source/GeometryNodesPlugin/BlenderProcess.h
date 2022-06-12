#pragma once

#include "GNData.h"

//#include <UnigineVector.h>
//#include <UnigineString.h>
//#include <UnigineHashMap.h>

//#include <QObject>
#include <QProcess>
//#include <QDataStream>
//#include <QByteArray>

namespace GeometryNodes
{

	enum class State
	{
		READY = 0,
		IDLE,
		BUSY,
		PARAMS,
		MESH
	};

	class BlenderProcess : public QProcess
	{
		Q_OBJECT
	public:
		BlenderProcess(QString id, QObject* parent = 0);
		~BlenderProcess() {}
		bool run(Unigine::String abspath);
		void request(const char* str);
		void requestMesh(Unigine::Vector<GNParam> params = Unigine::Vector<GNParam>(), Unigine::Vector<long long> hashes = Unigine::Vector<long long>());
		void updateParameter(GNParam& param, Unigine::Vector<long long> hashes, bool sync = false);
		State getState() { return state_; }
		Unigine::Vector<GNParam> getParameters();
		GNData getMeshData();

	private slots:
		void readResponse();

	private:
		State state_{ State::IDLE };
		GNData meshdata_;
		Unigine::Vector<GNParam> params_;

		struct UpdatedParameters
		{
			Unigine::HashMap<Unigine::String, GNParam> params;
			Unigine::Vector<long long> hashes;
			void clear()
			{
				params.clear();
				hashes.clear();
			}
		} pendingChanges_;

		void request_b(QByteArray data);
	};

} // namespace GeometryNodes