import 'package:flutter/material.dart';
import 'package:powermetric/providers/google_sign_in.dart';
import 'package:powermetric/widgets/sign_up_widget.dart';
import 'package:provider/provider.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:powermetric/screens/welcome_page.dart';

class WelcomeScreen extends StatelessWidget {
  const WelcomeScreen({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Scaffold(
        body: PageView(
      scrollDirection: Axis.vertical,
      children: [Screen1(), Screen2()],
    ));
  }
}

class Screen1 extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Stack(
      children: [
        Background(),
        MainContent(),
      ],
    );
  }
}

class MainContent extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    // ignore: prefer_const_constructors
    final textStyle = TextStyle(
        fontSize: 50, fontWeight: FontWeight.bold, color: Colors.white);
    return SafeArea(
      child: Column(
        children: [
          const SizedBox(
            height: 50,
          ),
          Text(
            'Bienvenido a PowerMetric',
            style: textStyle,
            textAlign: TextAlign.center,
          ),
          Expanded(child: Container()),
          // ignore: prefer_const_constructors
          Text(
            'Desliza hacia arriba',
            style: TextStyle(color: Colors.white),
          ),
          const Icon(
            Icons.keyboard_arrow_up,
            size: 100,
            color: Colors.white,
          )
        ],
      ),
    );
  }
}

class Background extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Container(
        color: const Color(0xff30BAD6),
        height: double.infinity,
        alignment: Alignment.topCenter,
        child: Image(image: AssetImage('assets/scroll-1.png')));
  }
}

class Screen2 extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      //backgroundColor: Color.fromRGBO(80, 194, 221, 1.0),
      backgroundColor: Colors.white,
      body: ChangeNotifierProvider(
        create: (context) => GoogleSignInProvider(),
        child: StreamBuilder(
          stream: FirebaseAuth.instance.authStateChanges(),
          builder: (context, snapshot) {
            final provider = Provider.of<GoogleSignInProvider>(context);
            if (provider.estaIngresado) {
              return buildLoading();
            } else if (snapshot.hasData) {
              return WelcomeScreen();
            } else {
              return SignUpWidget();
            }
          },
        ),
      ),
    );
  }
}

Widget buildLoading() {
  return Center(
    child: CircularProgressIndicator(),
  );
}
